#ifndef SPOTIFY_USER_H_
#define SPOTIFY_USER_H_

#include "index.h"

class User {
 public:
  static Handle<Object> NewInstance(sp_user *user);

  static Handle<Value> IsLoaded(Local<String> property,
                                const AccessorInfo& info);
  static Handle<Value> DisplayName(Local<String> property,
                                   const AccessorInfo& info);
  static Handle<Value> CanonicalName(Local<String> property,
                                     const AccessorInfo& info);
};

#endif  // SPOTIFY_USER_H_
