#include "./spotify.h"
#include "./session.h"

#include <libspotify/api.h>
#include <node.h>
#include <v8.h>

namespace spotify {

using namespace v8;

void Spotify::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<ObjectTemplate> t = ObjectTemplate::New();
  NODE_SET_METHOD(t, "withApplicationKey", WithApplicationKey);
  Local<Object> spotify = t->NewInstance();
  target->Set(String::NewSymbol("spotify"), spotify);
}

// Returns a function that can be used to login
Handle<Value> Spotify::WithApplicationKey(const Arguments& args) {
  HandleScope scope;

  // TODO(liesen): implement reading of application key
  /*
  if (args.Length() == 0) {
    return scope.Close(Undefined());
  }

  if (args[0]->IsArray()) {
    Local<Array> array = Array::Cast(*args[0]);
    int application_key_size = array->Length();
    fprintf(stderr, "application_key_size: %d\n", application_key_size);
    uint8_t application_key[application_key_size];

    for (int i = 0; i < application_key_size; i++) {
      Local<Object> x = array->CloneElementAt(i);
    }
  }
  */

  Local<FunctionTemplate> t = FunctionTemplate::New(Login);
  Local<Function> login = t->GetFunction();
  login->SetName(String::NewSymbol("login"));
  return scope.Close(login);
}

// See Session::Login 
Handle<Value> Spotify::Login(const Arguments& args) {
  HandleScope scope;
  Handle<Object> self = args.Holder();
  Handle<String> username = args[0]->ToString();
  Handle<String> password = args[1]->ToString();
  Handle<Function> error_callback = Local<Function>::Cast(args[2]);
  Handle<Function> success_callback = Local<Function>::Cast(args[3]);
  Session::Login(self, username, password, success_callback, error_callback);
  return scope.Close(Undefined());
}
}

extern "C" void init(v8::Handle<v8::Object> target) {
  spotify::Spotify::Initialize(target);
}

