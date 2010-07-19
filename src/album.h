#ifndef SPOTIFY_ALBUM_H_
#define SPOTIFY_ALBUM_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

class Album : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static void Initialize(v8::Handle<v8::Object> target);
  Album(sp_album *album);
  ~Album();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Local<v8::Object> New(sp_album *album);
  
  static v8::Handle<v8::Value> LoadedGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> ArtistGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
protected:
  void SetupBackingAlbum();
  sp_album* album_;
};

#endif
