#include "session.h"
#include "user.h"

#include <libspotify/api.h>
#include <node.h>
#include <node_events.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <v8.h>

using namespace node;
using namespace v8;

// log message struct queued on session->log_messages_q_
typedef struct log_message {
  struct log_message *next;
  const char *message;
} log_message_t;

static Persistent<String> logMessage_symbol;

// ----------------------------------------------------------------------------
// Helpers

#define NS_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

static inline char *NSValueToUTF8(Handle<Value> v) {
  char *p = new char[v->ToString()->Utf8Length()];
  v->ToString()->WriteUtf8(p);
  return p;
}

// ----------------------------------------------------------------------------
// libspotify callbacks

static void SpotifyRunloopTimerProcess(EV_P_ ev_timer *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  s->ProcessEvents();
}

static void SpotifyRunloopAsyncProcess(EV_P_ ev_async *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  s->ProcessEvents();
}

static void NotifyMainThread(sp_session* session) {
  // Called by a background thread (controlled by libspotify) when we need to
  // query sp_session_process_events, which is handled by
  // Session::ProcessEvents. ev_async_send queues a call on the main ev runloop.
  Session* s = static_cast<Session*>(sp_session_userdata(session));
  ev_async_send(EV_DEFAULT_UC_ &s->runloop_async_);
}

void Session::ProcessEvents() {
  int timeout = 0;
  // stop timer
  ev_timer_stop(EV_DEFAULT_UC_ &this->runloop_timer_);

  if (this->session_)
    sp_session_process_events(this->session_, &timeout);

  // schedule next tick
  ev_timer_set(&this->runloop_timer_, ((float)timeout)/1000.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ &this->runloop_timer_);
}

void Session::DequeueLogMessages() {
  log_message_t *msg;
  while ((msg = (log_message_t *)nt_atomic_dequeue(&this->log_messages_q_,
    offsetof(log_message_t, next))))
  {
    Local<Value> argv[] = { String::New(msg->message) };
    this->Emit(logMessage_symbol, 1, argv);
    delete msg->message;
    delete msg;
  }
}

static void SpotifyRunloopAsyncLogMessage(EV_P_ ev_async *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  s->DequeueLogMessages();
}

static void LogMessage(sp_session* session, const char* data) {
  Session* s = static_cast<Session*>(sp_session_userdata(session));
  if (pthread_self() == s->thread_id_) {
    // Called from the main runloop thread -- emit directly
    Local<Value> argv[] = { String::New(data) };
    s->Emit(logMessage_symbol, 1, argv);
  } else {
    // Called from a background thread -- queue and notify
    log_message_t *msg = new log_message_t;
    msg->message = (const char *)strdup(data);
    nt_atomic_enqueue(&s->log_messages_q_, msg, offsetof(log_message_t, next));
    // Signal we need to dequeue the message queue: SpotifyRunloopAsyncLogMessage
    ev_async_send(EV_DEFAULT_UC_ &s->logmsg_async_);
  }
}

static void MessageToUser(sp_session* session, const char* data) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->thread_id_ == pthread_self() /* or we will crash */);
  Local<Value> argv[] = { String::New(data) };
  s->Emit(String::New("message_to_user"), 1, argv);
}

static void LoggedOut(sp_session* session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->thread_id_ == pthread_self() /* or we will crash */);
  if (s->logout_callback_) {
    assert((*s->logout_callback_)->IsFunction());
    (*s->logout_callback_)->Call(s->handle_, 0, NULL);
    cb_destroy(s->logout_callback_);
    s->logout_callback_ = NULL;
  }
  ev_unref(EV_DEFAULT_UC);
  s->DequeueLogMessages();
}

static void LoggedIn(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->login_callback_ != NULL);
  assert((*s->login_callback_)->IsFunction());
  assert(s->thread_id_ == pthread_self() /* or we will crash */);
  if (error != SP_ERROR_OK) {
    Local<Value> argv[] = { Exception::Error(String::New(sp_error_message(error))) };
    (*s->login_callback_)->Call(s->handle_, 1, argv);
  } else {
    (*s->login_callback_)->Call(s->handle_, 0, NULL);
  }
  cb_destroy(s->login_callback_);
  s->login_callback_ = NULL;
}

static void MetadataUpdated(sp_session *session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->thread_id_ == pthread_self() /* or we will crash */);
  s->Emit(String::New("metadataUpdated"), 0, NULL);
}

static void ConnectionError(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->thread_id_ == pthread_self() /* or we will crash */);
  Local<Value> argv[] = { String::New(sp_error_message(error)) };
  s->Emit(String::New("connection_error"), 1, argv);
}

// ----------------------------------------------------------------------------
// Session implementation

Session::Session(sp_session* session)
  : session_(session)
  , thread_id_((pthread_t) -1)
  , login_callback_(NULL)
  , logout_callback_(NULL)
  , pc_(NULL)
{
  memset((void*)&this->log_messages_q_, 0, sizeof(nt_atomic_queue));
}

Session::~Session() {
  ev_timer_stop(EV_DEFAULT_UC_ &this->runloop_timer_);
  ev_async_stop(EV_DEFAULT_UC_ &this->runloop_async_);
  this->DequeueLogMessages();
  if (this->pc_) delete pc_;
  if (this->login_callback_) {
    cb_destroy(this->login_callback_);
    this->login_callback_ = NULL;
  }
  if (this->logout_callback_) {
    cb_destroy(this->logout_callback_);
    this->logout_callback_ = NULL;
  }
}

Handle<Value> Session::New(const Arguments& args) {
  Session* s = new Session(NULL);

  static sp_session_callbacks callbacks = {
    /* logged_in */             LoggedIn,
    /* logged_out */            LoggedOut,
    /* metadata_updated */      MetadataUpdated,
    /* connection_error */      ConnectionError,
    /* message_to_user */       MessageToUser,
    /* notify_main_thread */    NotifyMainThread,
    /* music_delivery */        NULL, // we don't play music
    /* play_token_lost */       NULL, // we don't play music
    /* log_message */           LogMessage,
    /* end_of_track */          NULL, // we don't play music
  };

  sp_session_config config = {
    /* api_version */           SPOTIFY_API_VERSION,
    /* cache_location */        NULL, // must be set and is below
    /* settings_location */     NULL, // must be set and is below
    /* application_key */       NULL, // optional but set below
    /* application_key_size */  0,
    /* user_agent */            NULL, // must be set and is below
    /* callbacks */             &callbacks,
    /* userdata */              s,
  };

  // appkey buffer
  uint8_t *keybuf = NULL;

  if (args.Length() > 0) {
    if (!args[0]->IsObject())
      NS_THROW(TypeError, "first argument must be an object");

    Local<Object> configuration = args[0]->ToObject();

    // applicationKey
    if (configuration->Has(String::New("applicationKey"))) {
      Local<Value> v = configuration->Get(String::New("applicationKey"));
      if (!v->IsArray()) NS_THROW(TypeError, "applicationKey must be an array of integers");
      Local<Array> a = Local<Array>::Cast(v);
      keybuf = new uint8_t[a->Length()];
      config.application_key_size = a->Length();
      for (int i = 0; i < a->Length(); i++) {
        keybuf[i] = a->Get(i)->Uint32Value();
      }
      config.application_key = keybuf;
    }

    // userAgent
    if (configuration->Has(String::New("userAgent"))) {
      Handle<Value> v = configuration->Get(String::New("userAgent"));
      if (!v->IsString()) NS_THROW(TypeError, "userAgent must be a string");
      config.user_agent = NSValueToUTF8(v);
    } else {
      // we strdup so we can safely free at dealloc
      config.user_agent = strdup("node-spotify");
    }

    // cacheLocation
    if (configuration->Has(String::New("cacheLocation"))) {
      Handle<Value> v = configuration->Get(String::New("cacheLocation"));
      if (!v->IsString()) NS_THROW(TypeError, "cacheLocation must be a string");
      config.cache_location = NSValueToUTF8(v);
    } else {
      // we strdup so we can safely free at dealloc
      config.cache_location = strdup(".spotify-cache");
    }

    // settingsLocation
    if (configuration->Has(String::New("settingsLocation"))) {
      Handle<Value> v = configuration->Get(String::New("settingsLocation"));
      if (!v->IsString()) NS_THROW(TypeError, "settingsLocation must be a string");
      config.settings_location = NSValueToUTF8(v);
    } else {
      // we strdup so we can safely free at dealloc
      config.settings_location = strdup(".spotify-settings");
    }
  }

  // ev_async for libspotify background thread to invoke processing on main
  s->runloop_async_.data = s;
  ev_async_init(&s->runloop_async_, SpotifyRunloopAsyncProcess);
  ev_async_start(EV_DEFAULT_UC_ &s->runloop_async_);
  ev_unref(EV_DEFAULT_UC); // don't let a lingering async ev keep the main loop

  // ev_timer for triggering libspotify periodic processing
  s->runloop_timer_.data = s;
  ev_timer_init(&s->runloop_timer_, SpotifyRunloopTimerProcess, 60.0, 0.0);
  ev_unref(EV_DEFAULT_UC);
  // Note: No need to start the timer as it's started by first invocation after
  // NotifyMainThread

  // ev_async for libspotify background thread to emit log message on main
  s->logmsg_async_.data = s;
  ev_async_init(&s->logmsg_async_, SpotifyRunloopAsyncLogMessage);
  ev_async_start(EV_DEFAULT_UC_ &s->logmsg_async_);
  ev_unref(EV_DEFAULT_UC); // don't let a lingering async ev keep the main loop

  sp_session* session;
  sp_error error = sp_session_init(&config, &session);

  // free temporary buffers in config
  if (keybuf) delete keybuf;
  #define F(_name_) if (config._name_) {\
    delete config._name_; config._name_ = NULL; }
  F(user_agent)
  F(cache_location)
  F(settings_location)
  #undef F

  if (error != SP_ERROR_OK)
    NS_THROW(Error, sp_error_message(error));

  s->session_ = session;
  s->thread_id_ = pthread_self();
  s->Wrap(args.Holder());
  return args.This();
}

Handle<Value> Session::Login(const Arguments& args) {
  HandleScope scope;

  if (args.Length() != 3) NS_THROW(TypeError, "login takes exactly 3 arguments");
  if (!args[0]->IsString()) NS_THROW(TypeError, "first argument must be a string");
  if (!args[1]->IsString()) NS_THROW(TypeError, "second argument must be a string");
  if (!args[2]->IsFunction()) NS_THROW(TypeError, "last argument must be a function");

  Session* s = Unwrap<Session>(args.This());

  #define SP_UTF8STR()

  char* username = NSValueToUTF8(args[0]);
  char* password = NSValueToUTF8(args[1]);

  // increase refcount for our timer event
  ev_ref(EV_DEFAULT_UC);

  // save login callback
  if (s->login_callback_) cb_destroy(s->login_callback_);
  s->login_callback_ = cb_persist(args[2]);

  sp_session_login(s->session_, username, password);

  delete username, password;
  return Undefined();
}

Handle<Value> Session::Logout(const Arguments& args) {
  HandleScope scope;

  if (args.Length() > 0 && !args[0]->IsFunction())
    NS_THROW(TypeError, "last argument must be a function");

  Session* s = Unwrap<Session>(args.This());

  // save logout callback
  if (args.Length() > 0) {
    if (s->logout_callback_) cb_destroy(s->logout_callback_);
    s->logout_callback_ = cb_persist(args[0]);
  }

  sp_session_logout(s->session_);
  return Undefined();
}

Handle<Value> Session::ConnectionStateGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  int connectionstate = sp_session_connectionstate(s->session_);
  return scope.Close(Integer::New(connectionstate));
}

extern Handle<Object> *g_spotify_module;

Handle<Value> Session::PlaylistContainerGetter(Local<String> property,
                                               const AccessorInfo& info)
{
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  if (!s->pc_)
    s->pc_ = PlaylistContainer::New(sp_session_playlistcontainer(s->session_));
  return scope.Close(s->pc_->handle_);
}

Handle<Value> Session::UserGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  sp_user* user = sp_session_user(s->session_);

  // The user property is exposed via a session object before the session
  // is connected/logged in, in which case the user object isn't initialized
  // and something weird has to be returned
  if (!user) {
    return Undefined();
  }

  return scope.Close(User::NewInstance(user));
}


void Session::Initialize(Handle<Object> target) {
  //printf("main T# %p\n", pthread_self());
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->Inherit(EventEmitter::constructor_template);

  NODE_SET_PROTOTYPE_METHOD(t, "logout", Logout);
  NODE_SET_PROTOTYPE_METHOD(t, "login", Login);

  Local<ObjectTemplate> instance_t = t->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::New("user"), UserGetter);
  instance_t->SetAccessor(String::New("connectionState"),
                          ConnectionStateGetter);
  instance_t->SetAccessor(String::New("playlists"), PlaylistContainerGetter);

  target->Set(String::New("Session"), t->GetFunction());

  logMessage_symbol = NODE_PSYMBOL("logMessage");
}
