#ifndef SPOTIFY_SESSION_H_
#define SPOTIFY_SESSION_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

namespace spotify {

class Session : public node::EventEmitter {
 public:
  // Login to Spotify. /error_callback/ is called with an error message if
  // login fails; otherwise /success_callback/ is called with the session
  // object
  static void Login(const v8::Handle<v8::Object> self,
                    v8::Handle<v8::String> username,
                    v8::Handle<v8::String> password,
                    v8::Handle<v8::Function> success_callback,
                    v8::Handle<v8::Function> error_callback);

  // Wraps a session in an v8::Object
  static v8::Local<v8::Object> New(sp_session* session);

 protected:
  // Log out of a session
  static v8::Handle<v8::Value> Logout(const v8::Arguments& args);

  static v8::Handle<v8::Value> ConnectionState(v8::Local<v8::String> property,
                                               const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> PlaylistContainer(v8::Local<v8::String> property,
                                                 const v8::AccessorInfo& info);

  // Gets the user associated with a session
  static v8::Handle<v8::Value> User(v8::Local<v8::String> property,
                                    const v8::AccessorInfo& info);

 protected:
  Session(sp_session* session) : session_(session) {
  }

  sp_session* session_;
  sp_session_callbacks* callbacks_;
  sp_session_config* config_;
};
}

#endif  // SPOTIFY_SESSION_H_
