#include "playlistcontainer.h"
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


namespace spotify {

using namespace node;
using namespace v8;

static pthread_t g_main_thread = (pthread_t) -1;
static int g_exit_code = -1;

static void signal_ignore(int signo) {
}

typedef struct login_callbacks {
  Handle<Object> self;
  Handle<Function> success_callback;
  Handle<Function> failure_callback;
} login_callbacks_t;

static void notify_main_thread(sp_session* session) {
  pthread_kill(g_main_thread, SIGIO);
}

static void session_loop(sp_session* session) {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGIO);

  while (g_exit_code < 0) { 
    int timeout = -1;
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    sp_session_process_events(session, &timeout);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    usleep(timeout * 1000);
  }
}

static void log_message(sp_session* session, const char* data) {
  fprintf(stderr, "sp: %s", data);
}

static void logged_out(sp_session* session) {
  g_exit_code = 0;
  log_message(session, "logged out\n");
}

static void LoggedIn(sp_session *session, sp_error error) {
  login_callbacks_t* login_callbacks = (login_callbacks_t*) sp_session_userdata(
      session);
  Handle<Object> self = login_callbacks->self; 

  if (error != SP_ERROR_OK) {
    Handle<Value> args[] = { String::New(sp_error_message(error)) };
    login_callbacks->failure_callback->Call(self, 1, args);
    return;
  }

  Handle<Value> args[] = { Session::New(session) };
  login_callbacks->success_callback->Call(self, 1, args); 
}

Local<Object> Session::New(sp_session* session) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New();
  t->SetClassName(NODE_PSYMBOL("spotify.Session"));
  // t->Inherit(EventEmitter::constructor_template);
  NODE_SET_PROTOTYPE_METHOD(t, "logout", Logout);

  Local<ObjectTemplate> instance_t = t->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(NODE_PSYMBOL("connectionState"), ConnectionState);
  instance_t->SetAccessor(NODE_PSYMBOL("playlistContainer"),
                          PlaylistContainer);
  instance_t->SetAccessor(NODE_PSYMBOL("user"), User);

  Local<Object> instance = t->GetFunction()->NewInstance();
  Session *s = new Session(session);
  s->Wrap(instance);
  return scope.Close(instance);
}

void Session::Login(const Handle<Object> self,
                    Handle<String> username,
                    Handle<String> password,
                    Handle<Function> success_callback,
                    Handle<Function> error_callback) {
  sp_session_callbacks callbacks = {
    /* logged_in */ LoggedIn,
    /* logged_out */ logged_out,
    /* metadata_updated */ NULL,
    /* connection_error */ NULL,
    /* message_to_user */ log_message,
    /* notify_main_thread */ notify_main_thread,
    /* music_delivery */ NULL,
    /* play_token_lost */ NULL,
    /* log_message */ log_message,
    /* end_of_track */ NULL,
  };

  login_callbacks_t login_callbacks = {
    self,
    success_callback,
    error_callback
  };

  sp_session_config config = {
    /* api_version */ SPOTIFY_API_VERSION,
    /* cache_location */ ".cache",
    /* settings_location */ ".settings",
    /* application_key */ g_appkey,
    /* application_key_size */ g_appkey_size,
    /* user agent */ "spotify-node",
    /* callbacks */ &callbacks,
    /* userdata */ &login_callbacks 
  };

  sp_session* session;
  sp_error error = sp_session_init(&config, &session);

  if (error != SP_ERROR_OK) {
    Handle<Value> argv[] = { String::New(sp_error_message(error)) };
    error_callback->Call(self, 1, argv);
    return;
  }

  g_main_thread = pthread_self();
  signal(SIGIO, &signal_ignore);

  sp_session_login(session,
                   *(String::Utf8Value(username)),
                   *(String::Utf8Value(password)));
  session_loop(session);
}

Handle<Value> Session::Logout(const Arguments& args) {
  HandleScope scope;
  sp_session* session = ObjectWrap::Unwrap<Session>(args.This())->session_;
  sp_session_logout(session);
  return scope.Close(Undefined());
}

Handle<Value> Session::ConnectionState(Local<String> property,
                                       const AccessorInfo& info) {
  HandleScope scope;
  sp_session* session = ObjectWrap::Unwrap<Session>(info.This())->session_;
  sp_connectionstate connectionstate = sp_session_connectionstate(session);
  return scope.Close(Integer::New(connectionstate));
}

Handle<Value> Session::PlaylistContainer(Local<String> property,
                                         const AccessorInfo& info) {
  HandleScope scope;
  sp_session* session = ObjectWrap::Unwrap<Session>(info.This())->session_;
  sp_playlistcontainer* pc = sp_session_playlistcontainer(session);
  return scope.Close(PlaylistContainer::New(pc));
}

Handle<Value> Session::User(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_session* session = ObjectWrap::Unwrap<Session>(info.This())->session_;
  sp_user* user = sp_session_user(session);
  return scope.Close(User::NewInstance(user));
}
}

