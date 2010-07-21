#ifndef SPOTIFY_TRACK_H_
#define SPOTIFY_TRACK_H_

#include "index.h"

class Album;

class Track : public EventEmitter {
 public:
  static Persistent<FunctionTemplate> constructor_template;

  static void Initialize(Handle<Object> target);

  explicit Track(sp_track *track);
  ~Track();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> New(sp_track *track);

  GETTER_H(LoadedGetter);
  GETTER_H(AlbumGetter);
  GETTER_H(ArtistsGetter);
  GETTER_H(UriGetter);
 protected:
  bool SetupBackingTrack();
  sp_track* track_;
  Album *album_;
};

#endif
