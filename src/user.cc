#include "user.h"

#include <v8.h>
#include <libspotify/api.h>
#include <node.h>

using namespace v8;

namespace spotify {

Handle<Object> User::NewInstance(sp_user *user) {
  Local<ObjectTemplate> t = ObjectTemplate::New();
  t->SetInternalFieldCount(1);

  t->SetAccessor(String::New("isLoaded"), IsLoaded);
  t->SetAccessor(String::New("displayName"), DisplayName);
  t->SetAccessor(String::New("canonicalName"), CanonicalName);

  Handle<Object> self = t->NewInstance();
  self->SetInternalField(0, External::New(user));
  return self;
}

Handle<Value> User::IsLoaded(Local<String> property, const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.Holder());
  bool is_loaded = sp_user_is_loaded(user);
  return Boolean::New(is_loaded);
}

Handle<Value> User::DisplayName(Local<String> property,
                                const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.Holder());
  const char* display_name = sp_user_display_name(user);
  return String::New(display_name);
}

Handle<Value> User::CanonicalName(Local<String> property,
                                  const AccessorInfo& info) {
  sp_user* user = node::ObjectWrap::Unwrap<sp_user>(info.Holder());
  const char* canonical_name = sp_user_canonical_name(user);
  return String::New(canonical_name);
}
}
