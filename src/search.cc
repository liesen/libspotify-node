#include "search.h"
#include "track.h"
#include "album.h"
#include "artist.h"

#include <string.h>

Persistent<FunctionTemplate> SearchResult::constructor_template;

// -----------------------------------------------------------------------------
// SearchResult implementation

SearchResult::SearchResult(sp_search *search)
  : node::EventEmitter()
  , search_(search)
{
}

Local<Object> SearchResult::New(sp_search *search) {
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  SearchResult *sr = ObjectWrap::Unwrap<SearchResult>(instance);
  sr->search_ = search;
  // call member "onsetup" (if function) to allow custom setup
  Handle<Value> setupFun = instance->Get(String::New("onsetup"));
  if (setupFun->IsFunction())
    Handle<Function>::Cast(setupFun)->Call(instance, 0, NULL);
  return instance;
}

Handle<Value> SearchResult::New(const Arguments& args) {
  (new SearchResult(NULL))->Wrap(args.This());
  return args.This();
}

Handle<Value> SearchResult::TracksGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult* s = Unwrap<SearchResult>(info.This());
  int count = sp_search_num_tracks(s->search_);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Track::New(sp_search_track(s->search_, i)));
  }
  return scope.Close(array);
}

Handle<Value> SearchResult::AlbumsGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult* s = Unwrap<SearchResult>(info.This());
  int count = sp_search_num_albums(s->search_);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Album::New(sp_search_album(s->search_, i)));
  }
  return scope.Close(array);
}

Handle<Value> SearchResult::ArtistsGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult* s = Unwrap<SearchResult>(info.This());
  int count = sp_search_num_artists(s->search_);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Artist::New(sp_search_artist(s->search_, i)));
  }
  return scope.Close(array);
}

Handle<Value> SearchResult::LoadedGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult *p = Unwrap<SearchResult>(info.This());
  return p->search_
    ? scope.Close(Boolean::New(sp_search_is_loaded(p->search_)))
    : Undefined();
}

Handle<Value> SearchResult::QueryGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult *p = Unwrap<SearchResult>(info.This());
  return p->search_
    ? scope.Close(String::New(sp_search_query(p->search_)))
    : Undefined();
}

Handle<Value> SearchResult::DidYouMeanGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult *p = Unwrap<SearchResult>(info.This());
  if (p->search_ && sp_search_is_loaded(p->search_)) {
    const char *s = sp_search_did_you_mean(p->search_);
    if (s && strlen(s) != 0)
      return scope.Close(String::New(s));
  }
  Undefined();
}

Handle<Value> SearchResult::TotalTracksGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult *p = Unwrap<SearchResult>(info.This());
  return (p->search_ && sp_search_is_loaded(p->search_))
    ? scope.Close(Integer::New(sp_search_total_tracks(p->search_)))
    : Undefined();
}

Handle<Value> SearchResult::URIGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  SearchResult *p = Unwrap<SearchResult>(info.This());
  if (!p->search_ || !sp_search_is_loaded(p->search_))
    return Undefined();
  int buflen = sizeof("spotify:search:")
    + strlen(sp_search_query(p->search_))
    + 2;
  char *buf = new char[buflen];
  sp_link *link = sp_link_create_from_search(p->search_);
  if (!link) {
    delete buf;
    return Undefined();
  }
  sp_link_as_string(link, buf, buflen);
  sp_link_release(link);
  Local<String> s = String::New(buf);
  delete buf;
  return scope.Close(s);
}

void SearchResult::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("SearchResult"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("loaded"), LoadedGetter);
  instance_t->SetAccessor(String::NewSymbol("tracks"), TracksGetter);
  instance_t->SetAccessor(String::NewSymbol("albums"), AlbumsGetter);
  instance_t->SetAccessor(String::NewSymbol("artists"), ArtistsGetter);
  instance_t->SetAccessor(String::NewSymbol("totalTracks"), TotalTracksGetter);
  instance_t->SetAccessor(String::NewSymbol("query"), QueryGetter);
  instance_t->SetAccessor(String::NewSymbol("didYouMean"), DidYouMeanGetter);
  instance_t->SetAccessor(String::NewSymbol("uri"), URIGetter);

  target->Set(String::New("SearchResult"), constructor_template->GetFunction());
}
