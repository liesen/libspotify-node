#include "user.h"

#include <v8.h>
#include <libspotify/api.h>
#include <node.h>

using namespace v8;

namespace spotify {

Handle<Object> User::NewInstance(sp_user *user) {
  HandleScope scope;
  Local<ObjectTemplate> t = ObjectTemplate::New();
  t->SetInternalFieldCount(1);

  t->SetAccessor(String::New("isLoaded"), IsLoaded);
  t->SetAccessor(String::New("displayName"), DisplayName);
  t->SetAccessor(String::New("canonicalName"), CanonicalName);

  Handle<Object> instance = t->NewInstance();
  instance->SetInternalField(0, External::New(user));
  return scope.Close(instance);
}

Handle<Value> User::IsLoaded(Local<String> property, const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.This());
  bool is_loaded = sp_user_is_loaded(user);
  return Boolean::New(is_loaded);
}

Handle<Value> User::DisplayName(Local<String> property,
                                const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.This());
  const char* display_name = sp_user_display_name(user);
  return String::New(display_name);
}

Handle<Value> User::CanonicalName(Local<String> property,
                                  const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.This());
  const char* canonical_name = sp_user_canonical_name(user);
  return String::New(canonical_name);
}
}
