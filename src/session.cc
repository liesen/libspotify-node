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

#include "appkey.c"

using namespace node;
using namespace v8;

#define SP_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;

static ev_timer g_runloop_timer;
static ev_check g_runloop_check;
static ev_async g_runloop_async;

static void signal_ignore(int signo) {
}

static void runloop_tick(EV_P_ ev_timer *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  printf("runloop_tick\n");
  s->ProcessEvents();
}

static void runloop_notify_tick(EV_P_ ev_async *w, int revents) {
  Session *s = static_cast<Session*>(w->data);
  //printf("runloop_notify_tick\n");
  s->ProcessEvents();
}

static void notify_main_thread(sp_session* session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  //printf("notify_main_thread\n");
  ev_async_send(EV_DEFAULT_UC_ &g_runloop_async);
}

void Session::ProcessEvents() {
  //printf("Session::ProcessEvents\n");
  int timeout = 0;
  // stop timer
  ev_timer_stop(EV_DEFAULT_UC_ &g_runloop_timer);
  
  if (this->session_)
    sp_session_process_events(this->session_, &timeout);
  //printf("next runloop_tick in %d ms\n", timeout);
  
  // schedule next tick
  ev_timer_set(&g_runloop_timer, ((float)timeout)/1000.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ &g_runloop_timer);
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
  assert(s->logout_callback_ != NULL);
  assert((*s->logout_callback_)->IsFunction());
  (*s->logout_callback_)->Call(Context::GetCurrent()->Global(), 0, NULL);
  cb_destroy(s->logout_callback_);
  s->logout_callback_ = NULL;
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
  instance_t->SetAccessor(String::New("connectionState"), ConnectionStateGetter);
  
  target->Set(NODE_PSYMBOL("Session"), t->GetFunction());
}

Session::~Session() {
  ev_timer_stop(EV_DEFAULT_UC_ &g_runloop_timer);
  ev_async_stop(EV_DEFAULT_UC_ &g_runloop_async);
  printf("session destroyed\n");
}

Handle<Value> Session::New(const Arguments& args) {
  Session* s = new Session(NULL);
  sp_session_callbacks callbacks = {
    /* logged_in */ LoggedIn,
    /* logged_out */ LoggedOut,
    /* metadata_updated */ NULL,
    /* connection_error */ ConnectionError,
    /* message_to_user */ MessageToUser,
    /* notify_main_thread */ notify_main_thread,
    /* music_delivery */ NULL,
    /* play_token_lost */ NULL,
    /* log_message */ LogMessage,
    /* end_of_track */ NULL,
  };
  sp_session_config config = {
    /* api_version */ SPOTIFY_API_VERSION,
    /* cache_location */ ".cache",
    /* settings_location */ ".settings",
    /* application_key */ g_appkey,
    /* application_key_size */ g_appkey_size,
    /* user agent */ "spotify-node",
    /* callbacks */ &callbacks,
    /* userdata */ s 
  };

  if (args.Length() > 0) {
    if (!args[0]->IsObject()) {
      return ThrowException(Exception::TypeError(
        String::New("first argument must be an object")));
    }

    Local<Object> configuration = args[0]->ToObject();

    if (configuration->Has(String::New("applicationKey"))) {
      Handle<Value> applicationKey = configuration->Get(String::New("applicationKey"));

      if (!applicationKey->IsString()) {
        return ThrowException(Exception::TypeError(
            String::New("application key must be a string")));
      }

      config.application_key = g_appkey;
      config.application_key_size = g_appkey_size;
    }
  }
  
  // register in the runloop
  g_runloop_async.data = s;
  ev_async_init(&g_runloop_async, runloop_notify_tick);
  ev_async_start(EV_DEFAULT_UC_ &g_runloop_async);
  ev_unref(EV_DEFAULT_UC);
  // todo: g_runloop_timer --> instance member
  g_runloop_timer.data = s;
  ev_timer_init(&g_runloop_timer, runloop_tick, 60.0, 0.0);
  ev_unref(EV_DEFAULT_UC);
  // no need to start this as it's started by first invocation after notify_main_thread
  //ev_timer_start(EV_DEFAULT_UC_ &g_runloop_timer);

  sp_session* session;
  sp_error error = sp_session_init(&config, &session);

  if (error != SP_ERROR_OK) {
    fprintf(stderr, "%s", sp_error_message(error));
    return Undefined();
  }

  s->session_ = session;
  s->thread_id_ = pthread_self();
  signal(SIGIO, &signal_ignore);
  s->Wrap(args.Holder());
  return args.This();
}

Handle<Value> Session::Login(const Arguments& args) {
  HandleScope scope;
  
  if (args.Length() != 3) SP_THROW(TypeError, "login takes exactly 3 arguments");
  if (!args[0]->IsString()) SP_THROW(TypeError, "first argument must be a string");
  if (!args[1]->IsString()) SP_THROW(TypeError, "second argument must be a string");
  if (!args[2]->IsFunction()) SP_THROW(TypeError, "last argument must be a function");

  Session* s = Unwrap<Session>(args.This());

  // *(String::Utf8Value(args[0]->ToString()) didn't work out the way I thought
  // it would, so...
  char* username = new char[args[0]->ToString()->Utf8Length()];
  args[0]->ToString()->WriteUtf8(username);

  char* password = new char[args[1]->ToString()->Utf8Length()];
  args[1]->ToString()->WriteUtf8(password);

  // save login callback
  if (s->login_callback_) cb_destroy(s->login_callback_);
  s->login_callback_ = cb_persist(args[2]);
  
  ev_ref(EV_DEFAULT_UC);
  sp_session_login(s->session_, username, password);
  
  //s->Loop();

  return Undefined();
}

Handle<Value> Session::Logout(const Arguments& args) {
  HandleScope scope;
  
  if (args.Length() != 1) SP_THROW(TypeError, "login takes exactly one arguments");
  if (!args[0]->IsFunction()) SP_THROW(TypeError, "last argument must be a function");

  Session* s = Unwrap<Session>(args.This());
  
  // save logout callback
  if (s->logout_callback_) cb_destroy(s->login_callback_);
  s->logout_callback_ = cb_persist(args[0]);
  
  sp_session_logout(s->session_);
  return Undefined();
}

Handle<Value> Session::ConnectionStateGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  int connectionstate = sp_session_connectionstate(s->session_);
  return scope.Close(Integer::New(connectionstate));
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
