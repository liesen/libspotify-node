#ifndef SPOTIFY_SEARCH_H_
#define SPOTIFY_SEARCH_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

class SearchResult : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  
  static void Initialize(v8::Handle<v8::Object> target);
  SearchResult(sp_search* search);
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Local<v8::Object> New(sp_search *search);

  static v8::Handle<v8::Value> LoadedGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> TracksGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> AlbumsGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> ArtistsGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> TotalTracksGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> QueryGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> DidYouMeanGetter(v8::Local<v8::String> property, const v8::AccessorInfo& info);
protected:
  sp_search* search_;
};

#endif
