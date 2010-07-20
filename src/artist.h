#ifndef SPOTIFY_ARTIST_H_
#define SPOTIFY_ARTIST_H_

#include "index.h"

class Artist : public EventEmitter {
public:
  static Persistent<FunctionTemplate> constructor_template;
  static void Initialize(Handle<Object> target);
  Artist(sp_artist *artist);
  ~Artist();
  static Handle<Value> New(const Arguments& args);
  static Local<Object> New(sp_artist *artist);

  static Handle<Value> LoadedGetter(Local<String> property, const AccessorInfo& info);
  static Handle<Value> URIGetter(Local<String> property, const AccessorInfo& info);
protected:
  void SetupBackingArtist();
  sp_artist* artist_;
};

#endif
