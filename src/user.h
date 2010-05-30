#ifndef SPOTIFY_USER_H_
#define SPOTIFY_USER_H_

#include <v8.h>
#include <libspotify/api.h>
#include <node.h>

namespace spotify {

class User {
 public:
  static v8::Handle<v8::Object> NewInstance(sp_user *user);

  static v8::Handle<v8::Value> IsLoaded(v8::Local<v8::String> property,
                                        const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> DisplayName(v8::Local<v8::String> property,
                                           const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> CanonicalName(v8::Local<v8::String> property,
                                             const v8::AccessorInfo& info);
};
}

#endif  // SPOTIFY_USER_H_
