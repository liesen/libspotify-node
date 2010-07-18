#include "session.h"
#include "user.h"

#include <ev.h>
#include <libspotify/api.h>
#include <node.h>
#include <node_events.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <v8.h>

#include "appkey.c"

using namespace node;
using namespace v8;

#define SP_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;

static int g_exit_code = -1;

static ev_timer g_runloop_timer;
static ev_check g_runloop_check;

static void signal_ignore(int signo) {
}

static void runloop_tick(EV_P_ ev_timer *w, int revents) {
  int timeout = -1;
  Session *s = static_cast<Session*>(w->data);
  
  // stop timer
  ev_timer_stop(EV_DEFAULT_UC_ w);
  
  //pthread_sigmask(SIG_BLOCK, &s->_runloopSigset, NULL);
  printf("runloop_tick\n");
  sp_session_process_events(s->session_, &timeout);
  //printf("next runloop_tick in %.1f s\n", ((float)timeout)/1000.0);
  //pthread_sigmask(SIG_UNBLOCK, &s->_runloopSigset, NULL);
  
  // schedule next tick
  ev_timer_set(w, ((float)timeout)/1000.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ w);
}

static void notify_main_thread(sp_session* session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  printf("notify_main_thread\n");
  ev_timer_stop(EV_DEFAULT_UC_ &g_runloop_timer);
  ev_timer_set(&g_runloop_timer, 0.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ &g_runloop_timer);
}

void Session::Loop() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGIO);

  while (g_exit_code < 0) { 
    int timeout = -1;
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    sp_session_process_events(this->session_, &timeout);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    printf("next runloop_tick in %d ms\n", timeout);
    usleep(timeout * 1000);
    printf("runloop again\n");
  }
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

static void LoggedOut(sp_session* session) {
  g_exit_code = 0;
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  ev_timer_stop(EV_DEFAULT_UC_ &g_runloop_timer);
  s->Emit(String::New("logged_out"), 0, NULL);
}

static void LoggedIn(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  // todo: simplify this
  if (error != SP_ERROR_OK) {
    Local<Value> argv[] = { Exception::Error(String::New(sp_error_message(error))) };
    s->Emit(String::New("logged_in"), 1, argv);
  } else {
    s->Emit(String::New("logged_in"), 0, NULL);
  }
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

Handle<Value> Session::New(const Arguments& args) {
  Session* s = new Session(NULL);
  sp_session_callbacks callbacks = {
    /* logged_in */ LoggedIn,
    /* logged_out */ LoggedOut,
    /* metadata_updated */ NULL,
    /* connection_error */ ConnectionError,
    /* message_to_user */ LogMessage,
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
  // todo: g_runloop_timer --> instance member
  ev_timer_init(&g_runloop_timer, runloop_tick, 0.0, 0.0);
  ev_timer_start(EV_DEFAULT_UC_ &g_runloop_timer);
  g_runloop_timer.data = s;

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

  // todo: macrofy this repetitive task (of adding a listener)
  Local<Function> addListener = Function::Cast(*args.This()->Get(String::New("addListener")));
  Local<Value> argv[] = {
    String::New("logged_in"),
    args[2]
  };
  addListener->Call(args.This(), 2, argv);
  
  sp_session_login(s->session_, username, password);
  
  //s->Loop();

  return Undefined();
}

Handle<Value> Session::Logout(const Arguments& args) {
  HandleScope scope;
  
  if (args.Length() != 1) SP_THROW(TypeError, "login takes exactly one arguments");
  if (!args[0]->IsFunction()) SP_THROW(TypeError, "last argument must be a function");
  
  Session* s = Unwrap<Session>(args.This());

  // todo: macrofy this repetitive task (of adding a listener)
  Local<Function> addListener = Function::Cast(*args.This()->Get(String::New("addListener")));
  Local<Value> argv[] = { String::New("logged_out"), args[0] };
  addListener->Call(args.This(), 2, argv);

  sp_session_logout(s->session_);
  return Undefined();
}

Handle<Value> Session::ConnectionStateGetter(Local<String> property,
                                       const AccessorInfo& info) {
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
