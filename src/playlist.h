#ifndef SPOTIFY_PLAYLIST_H_
#define SPOTIFY_PLAYLIST_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>


using namespace v8;
using namespace node;

class Playlist : public node::EventEmitter {
 public:
  static void Initialize(v8::Handle<v8::Object> target);
  
  Playlist(sp_playlist* playlist);
  ~Playlist();
  
  static Handle<Value> New(const Arguments& args);
  static Handle<Value> New(sp_playlist *playlist);

  static Handle<Value> LoadedGetter(Local<String> property,
                                    const AccessorInfo& info);
  static Handle<Value> NameGetter(Local<String> property,
                                  const AccessorInfo& info);
  static Handle<Value> URIGetter(Local<String> property,
                                 const AccessorInfo& info);

 protected:
  sp_playlist* playlist_;
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif
