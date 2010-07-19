#ifndef SPOTIFY_TRACK_H_
#define SPOTIFY_TRACK_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

class Album;

class Track : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static void Initialize(v8::Handle<v8::Object> target);
  Track(sp_track *track);
  ~Track();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> New(sp_track *track);
  
  static v8::Handle<v8::Value> LoadedGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> AlbumGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> ArtistsGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> URIGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
protected:
  bool SetupBackingTrack();
  sp_track* track_;
  Album *album_;
};

#endif
