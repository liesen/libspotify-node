#ifndef SPOTIFY_SESSION_H_
#define SPOTIFY_SESSION_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

class Session : public node::EventEmitter {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(const v8::Arguments& args);

  static v8::Handle<v8::Value> Login(const v8::Arguments& args);

  static v8::Handle<v8::Value> Logout(const v8::Arguments& args);

  static v8::Handle<v8::Value> ConnectionStateGetter(
      v8::Local<v8::String> property,
      const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> PlaylistContainerGetter(
      v8::Local<v8::String> property,
      const v8::AccessorInfo& info);

  // Gets the user associated with a session
  static v8::Handle<v8::Value> UserGetter(v8::Local<v8::String> property,
                                          const v8::AccessorInfo& info);

  void EmitLogMessage(const char* message);
  void ProcessEvents();

  Session(sp_session* session)
    : session_(session)
    , thread_id_((pthread_t) -1)
    , login_callback_(NULL)
    , logout_callback_(NULL)
    {}
  ~Session();

  sp_session* session_;
  pthread_t thread_id_;
  v8::Persistent<v8::Function> *logout_callback_;
  v8::Persistent<v8::Function> *login_callback_;
};

#endif  // SPOTIFY_SESSION_H_
