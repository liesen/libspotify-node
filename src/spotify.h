#ifndef SPOTIFY_H_
#define SPOTIFY_H_

#include <libspotify/api.h>
#include <node.h>
#include <v8.h>

namespace spotify {

class Spotify {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  static v8::Handle<v8::Value> Login(const v8::Arguments& args);
  static v8::Handle<v8::Value> WithApplicationKey(const v8::Arguments& args);

  static void Login(const v8::Handle<v8::Object> self,
                    v8::Local<v8::String> username,
                    v8::Local<v8::String> password,
                    v8::Handle<v8::Function> error_callback,
                    v8::Handle<v8::Function> session_callback);
};
}

#endif  // SPOTIFY_H_
