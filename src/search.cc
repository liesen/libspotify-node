#include "search.h"
#include "track.h"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> SearchResult::constructor_template;

// -----------------------------------------------------------------------------
// SearchResult implementation

SearchResult::SearchResult(sp_search *search)
  : node::EventEmitter()
  , search_(search)
{
}

Handle<Value> SearchResult::New(sp_search *search) {
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


void SearchResult::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("SearchResult"));
  constructor_template->Inherit(EventEmitter::constructor_template);
  
  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(NODE_PSYMBOL("tracks"), TracksGetter);
  //instance_t->SetAccessor(String::NewSymbol("isLoaded"), IsLoaded);
  /*instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(TrackGetter,
                                        TrackSetter,
                                        TrackQuery,
                                        TrackDeleter,
                                        TrackEnumerator);*/

  target->Set(String::New("SearchResult"), constructor_template->GetFunction());
}
