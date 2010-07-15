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

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;


using namespace node;
using namespace v8;

static int g_exit_code = -1;

static void signal_ignore(int signo) {
}

static void notify_main_thread(sp_session* session) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  pthread_kill(s->thread_id_, SIGIO);
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
    usleep(timeout * 1000);
  }
}

void Session::EmitLogMessage(const char* message) {
  HandleScope scope;
  Local<Value> argv[] = { String::New(message) };
  Emit(String::New("log_message"), 1, argv);
}

static void LogMessage(sp_session* session, const char* data) {
  fprintf(stderr, "sp: %s", data);
}

static void LoggedOut(sp_session* session) {
  g_exit_code = 0;
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  s->Emit(String::New("logged_out"), 0, NULL);
}

static void LoggedIn(sp_session* session, sp_error error) {
  Session* s = reinterpret_cast<Session*>(sp_session_userdata(session));
  Local<Value> argv[] = { Integer::New(error) };
  s->Emit(String::New("logged_in"), 1, argv);
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
  instance_t->SetAccessor(String::New("user"), User);
  instance_t->SetAccessor(String::New("connectionState"), ConnectionState);
  
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

  sp_session* session;
  sp_error error = sp_session_init(&config, &session);

  if (error != SP_ERROR_OK) {
    fprintf(stderr, "%s", sp_error_message(error));
    return Undefined();
  }

  s->session_ = session;
  s->thread_id_ = pthread_self();
  s->Wrap(args.This());
  signal(SIGIO, &signal_ignore);
  return args.This();
}

void Session::Login(const char* username, const char* password) {
  sp_session_login(this->session_, username, password);
}

Handle<Value> Session::Login(const Arguments& args) {
  HandleScope scope;

  if (!args.Length() > 0) {
    // No arguments: bail
  }

  if (!args[0]->IsString()) {
    // First argument not a string (not a user name): bail
  }

  // *(String::Utf8Value(args[0]->ToString()) didn't work out the way I thought
  // it would, so...
  char* username = new char[args[0]->ToString()->Utf8Length()];
  args[0]->ToString()->WriteUtf8(username);

  if (!args.Length() > 1) {
    // No password argument: bail
  }

  if (!args[1]->IsString()) {
    // Password argument not a string: bail
  }

  char* password = new char[args[1]->ToString()->Utf8Length()];
  args[1]->ToString()->WriteUtf8(password);

  Session* s = Unwrap<Session>(args.This());
  s->Login(username, password);
  s->Loop();
  return scope.Close(Undefined());
}

Handle<Value> Session::Logout(const Arguments& args) {
  HandleScope scope;
  Session* s = Unwrap<Session>(args.This());
  s->Logout();
  return scope.Close(Undefined());
}

void Session::Logout() {
  sp_session_logout(this->session_);
}

Handle<Value> Session::ConnectionState(Local<String> property,
                                       const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  int connectionstate = sp_session_connectionstate(s->session_);
  return scope.Close(Integer::New(connectionstate));
}

Handle<Value> Session::User(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Session* s = Unwrap<Session>(info.This());
  sp_user* user = sp_session_user(s->session_);

  // The user property is exposed via a session object before the session
  // is connected/logged in, in which case the user object isn't initialized
  // and something weird has to be returned
  if (!user) {
    return scope.Close(Undefined());
  }

  return scope.Close(User::NewInstance(user));
}
