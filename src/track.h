#ifndef SPOTIFY_TRACK_H_
#define SPOTIFY_TRACK_H_

#include "index.h"

class Album;

class Track : public EventEmitter {
public:
  static Persistent<FunctionTemplate> constructor_template;
  static void Initialize(Handle<Object> target);
  Track(sp_track *track);
  ~Track();
  static Handle<Value> New(const Arguments& args);
  static Handle<Value> New(sp_track *track);
  
  static Handle<Value> LoadedGetter(Local<String> property, const AccessorInfo& info);
  static Handle<Value> AlbumGetter(Local<String> property, const AccessorInfo& info);
  static Handle<Value> ArtistsGetter(Local<String> property, const AccessorInfo& info);
  static Handle<Value> URIGetter(Local<String> property, const AccessorInfo& info);
protected:
  bool SetupBackingTrack();
  sp_track* track_;
  Album *album_;
};

#endif
