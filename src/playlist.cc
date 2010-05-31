#include "playlist.h"

#include <v8.h>
#include <node.h>
#include <libspotify/api.h>

namespace spotify {

using namespace v8;
using namespace node;

Handle<Value> Playlist::New(sp_playlist *playlist) {
  HandleScope scope;
  Local<ObjectTemplate> instance_t = ObjectTemplate::New(); 
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("isLoaded"), IsLoaded);
  Handle<Object> instance = instance_t->NewInstance();
  (new Playlist(playlist))->Wrap(instance);
  return scope.Close(instance);
}

Handle<Value> Playlist::IsLoaded(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  return scope.Close(Boolean::New(sp_playlist_is_loaded(playlist)));
}

}

