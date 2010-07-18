#include "playlistcontainer.h"
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

// -------
// Helpers

#define NS_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

static inline char *NSValueToUTF8(Handle<Value> v) {
  char *p = new char[v->ToString()->Utf8Length()];
  v->ToString()->WriteUtf8(p);
  return p;
}

static void runloop_tick(EV_P_ ev_timer *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  s->ProcessEvents();
}

static void runloop_notify_tick(EV_P_ ev_async *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  s->ProcessEvents();
}

static void notify_main_thread(sp_session* session) {
  // Called by a background thread (controlled by libspotify) when we need to
  // query sp_session_process_events, which is handled by
  // Session::ProcessEvents. ev_async_send queues a call on the main ev runloop.
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  ev_async_send(EV_DEFAULT_UC_ &s->runloop_async_);
}

void Session::ProcessEvents() {
  int timeout = 0;
  // stop timer
  ev_timer_stop(EV_DEFAULT_UC_ &this->runloop_timer_);
  
  if (this->session_)
    sp_session_process_events(this->session_, &timeout);
  //printf("next runloop_tick in %d ms\n", timeout);
  
  // schedule next tick
  ev_timer_set(&this->runloop_timer_, ((float)timeout)/1000.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ &this->runloop_timer_);
}

void Session::EmitLogMessage(const char* message) {
  HandleScope scope;
  Handle<Value> argv[1];
  argv[0] = Handle<Value>(String::New(message));
  Emit(String::New("log_message"), 1, argv);
}

static void LogMessage(sp_session* session, const char* data) {
  printf("log_message: %s", data);
  fflush(stdout);
}

static void MessageToUser(sp_session* session, const char* data) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  HandleScope scope;
  Local<Value> argv[] = { String::New(data) };
  s->Emit(String::New("message_to_user"), 1, argv);
}

static void LoggedOut(sp_session* session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));  
  if (s->logout_callback_) {
    assert((*s->logout_callback_)->IsFunction());
    (*s->logout_callback_)->Call(Context::GetCurrent()->Global(), 0, NULL);
    cb_destroy(s->logout_callback_);
    s->logout_callback_ = NULL;
  }
  ev_unref(EV_DEFAULT_UC);
}

static void LoggedIn(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  assert(s->login_callback_ != NULL);
  assert((*s->login_callback_)->IsFunction());
  if (error != SP_ERROR_OK) {
    Local<Value> argv[] = { Exception::Error(String::New(sp_error_message(error))) };
    (*s->login_callback_)->Call(Context::GetCurrent()->Global(), 1, argv);
  } else {
    (*s->login_callback_)->Call(Context::GetCurrent()->Global(), 0, NULL);
  }
  cb_destroy(s->login_callback_);
  s->login_callback_ = NULL;
}

static void ConnectionError(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  Local<Value> argv[] = { String::New(sp_error_message(error)) };
  s->Emit(String::New("connection_error"), 1, argv);
}


void Session::Initialize(Handle<Object> target) {
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
  
  target->Set(NODE_PSYMBOL("Session"), t->GetFunction());
}

Session::~Session() {
  ev_timer_stop(EV_DEFAULT_UC_ &this->runloop_timer_);
  ev_async_stop(EV_DEFAULT_UC_ &this->runloop_async_);
  printf("session destroyed\n");
}

Handle<Value> Session::New(const Arguments& args) {
  Session* s = new Session(NULL);
  
  static sp_session_callbacks callbacks = {
    /* logged_in */             LoggedIn,
    /* logged_out */            LoggedOut,
    /* metadata_updated */      NULL,
    /* connection_error */      ConnectionError,
    /* message_to_user */       MessageToUser,
    /* notify_main_thread */    notify_main_thread,
    /* music_delivery */        NULL, // we don't play music
    /* play_token_lost */       NULL, // we don't play music
    /* log_message */           LogMessage,
    /* end_of_track */          NULL, // we don't play music
  };

  sp_session_config config = {
    /* api_version */           SPOTIFY_API_VERSION,
    /* cache_location */        ".cache",
    /* settings_location */     ".settings",
    /* application_key */       NULL,
    /* application_key_size */  0,
    /* user_agent */            "node-spotify",
    /* callbacks */             &callbacks,
    /* userdata */              s,
  };

  if (args.Length() > 0) {
    if (!args[0]->IsObject())
      NS_THROW(TypeError, "first argument must be an object");

    Local<Object> configuration = args[0]->ToObject();

    // applicationKey
    if (configuration->Has(String::New("applicationKey"))) {
      Local<Value> v = configuration->Get(String::New("applicationKey"));
      if (!v->IsArray()) NS_THROW(TypeError, "applicationKey must be an array of integers");
      Local<Array> a = Local<Array>::Cast(v);
      uint8_t *keybuf = new uint8_t[a->Length()];
      config.application_key_size = a->Length();
      for (int i = 0; i < a->Length(); i++) {
        keybuf[i] = a->Get(i)->Uint32Value();
      }
      config.application_key = keybuf;
      // todo: save ref to keybuf so we can free it at dealloc
    }
    
    // userAgent
    if (configuration->Has(String::New("userAgent"))) {
      Handle<Value> v = configuration->Get(String::New("userAgent"));
      if (!v->IsString()) NS_THROW(TypeError, "userAgent must be a string");
      config.user_agent = NSValueToUTF8(v); // todo: free this at dealloc
    }
  }
  
  // register in the runloop
  s->runloop_async_.data = s;
  ev_async_init(&s->runloop_async_, runloop_notify_tick);
  ev_async_start(EV_DEFAULT_UC_ &s->runloop_async_);
  ev_unref(EV_DEFAULT_UC);
  s->runloop_timer_.data = s;
  ev_timer_init(&s->runloop_timer_, runloop_tick, 60.0, 0.0);
  ev_unref(EV_DEFAULT_UC);
  // Note: No need to start the timer as it's started by first invocation after
  // notify_main_thread

  sp_session* session;
  sp_error error = sp_session_init(&config, &session);

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
    if (s->logout_callback_) cb_destroy(s->login_callback_);
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


Handle<Value> Session::PlaylistContainerGetter(Local<String> property,
                                               const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  sp_playlistcontainer* pc = sp_session_playlistcontainer(s->session_);
  return scope.Close(PlaylistContainer::New(pc));
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
