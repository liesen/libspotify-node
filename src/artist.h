#ifndef SPOTIFY_ARTIST_H_
#define SPOTIFY_ARTIST_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

class Artist : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static void Initialize(v8::Handle<v8::Object> target);
  Artist(sp_artist *artist);
  ~Artist();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Local<v8::Object> New(sp_artist *artist);
  
  static v8::Handle<v8::Value> LoadedGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> URIGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
protected:
  void SetupBackingArtist();
  sp_artist* artist_;
};

#endif
