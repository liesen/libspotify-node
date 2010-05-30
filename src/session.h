#ifndef SPOTIFY_SESSION_H_
#define SPOTIFY_SESSION_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

namespace spotify {

using namespace v8;
using namespace node;

class Session : public EventEmitter {
 public:
  // Login to Spotify. /error_callback/ is called with an error message if 
  // login fails; otherwise /success_callback/ is called with the session
  // object
  static void Login(const Handle<Object> self,
             Handle<String> username,
             Handle<String> password,
             Handle<Function> success_callback,
             Handle<Function> error_callback);

  // Wraps a session in an Object
  static Local<Object> NewInstance(sp_session* session);

 protected:
  // Log out of a session
  static Handle<Value> Logout(const Arguments& args);

  static Handle<Value> ConnectionState(Local<String> property,
                                       const AccessorInfo& info);

  static Handle<Value> PlaylistContainer(Local<String> property,
                                         const AccessorInfo& info);

  // Gets the user associated with a session
  static Handle<Value> User(Local<String> property, const AccessorInfo& info);

 private:
  Session(sp_session* session) : session_(session) {
  }

  sp_session* session_;
  sp_session_callbacks* callbacks_;
  sp_session_config* config_;

};

}

#endif
