#include "playlistcontainer.h"
#include "playlist.h"

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> PlaylistContainer::constructor_template;

// -----------------------------------------------------------------------------
// libspotify callbacks

static void PlaylistAdded(sp_playlistcontainer *pc,
                          sp_playlist *playlist,
                          int position,
                          void *userdata)
{
  // this is called on the main thread
  // todo: keep refs to playlist objects? ...or maybe only emit if there are
  // listeners? Creating all these playlists is probably hard work.
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = { Playlist::New(playlist), Integer::New(position) };
  p->Emit(String::New("playlistAdded"), 2, argv);
}

static void PlaylistRemoved(sp_playlistcontainer *pc,
                            sp_playlist *playlist,
                            int position,
                            void *userdata)
{
  // this is called on the main thread
  // todo: keep refs to playlist objects? ...or maybe only emit if there are
  // listeners? Creating all these playlists is probably hard work.
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = { Playlist::New(playlist), Integer::New(position) };
  p->Emit(String::New("playlistRemoved"), 2, argv);
}

static void PlaylistContainerLoaded(sp_playlistcontainer* pc, void* userdata) {
  // this is called on the main thread
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  p->Emit(String::New("load"), 0, NULL);
}

// -----------------------------------------------------------------------------
// PlaylistContainer implementation

static sp_playlistcontainer_callbacks callbacks = {
  PlaylistAdded,
  PlaylistRemoved,
  NULL,
  PlaylistContainerLoaded
};

PlaylistContainer::PlaylistContainer(sp_playlistcontainer* playlist_container)
  : EventEmitter(), playlist_container_(playlist_container) {
  if (this->playlist_container_)
    sp_playlistcontainer_add_callbacks(this->playlist_container_, &callbacks, this);
}

Handle<Value> PlaylistContainer::New(sp_playlistcontainer *playlist_container) {
  HandleScope scope;
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  PlaylistContainer *pc = ObjectWrap::Unwrap<PlaylistContainer>(instance);
  pc->playlist_container_ = playlist_container;
  sp_playlistcontainer_add_callbacks(pc->playlist_container_, &callbacks, pc);
  return instance;
}

Handle<Value> PlaylistContainer::New(const Arguments& args) {
  HandleScope scope;
  PlaylistContainer* pc = new PlaylistContainer(
    args.Length() > 0 && args[0]->IsExternal()
      ? (sp_playlistcontainer *)External::Unwrap(args[0])
      : NULL);
  pc->Wrap(args.This());
  return args.This();
}

Handle<Value> PlaylistContainer::LengthGetter(Local<String> property,
                                              const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int num_playlists = pc->num_playlists();
  return scope.Close(Integer::New(num_playlists));
}

Handle<Value> PlaylistContainer::PlaylistGetter(uint32_t index, const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  sp_playlist* playlist = sp_playlistcontainer_playlist(
      pc->playlist_container_, index);

  if (playlist == NULL)
    return scope.Close(Undefined());

  return scope.Close(Playlist::New(playlist));
}

Handle<Value> PlaylistContainer::PlaylistSetter(uint32_t index,
                                                Local<Value> value,
                                                const AccessorInfo& info) {
  return Handle<Value>();
}

Handle<Boolean> PlaylistContainer::PlaylistDeleter(uint32_t index,
                                                   const AccessorInfo& info) {
  return Handle<Boolean>();
}


Handle<Boolean> PlaylistContainer::PlaylistQuery(uint32_t index,
                                                 const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int num_playlists = pc->num_playlists(); 
  return scope.Close(Boolean::New(index < num_playlists));
}

Handle<Array> PlaylistContainer::PlaylistEnumerator(const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int num_playlists = pc->num_playlists(); 
  Local<Array> playlists = Array::New(num_playlists);

  for (int i = 0; i < num_playlists; i++) {
    sp_playlist* playlist = sp_playlistcontainer_playlist(
        pc->playlist_container_, i);
    playlists->Set(i, Playlist::New(playlist));
  }

  return scope.Close(playlists);
}

void PlaylistContainer::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("PlaylistContainer"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(PlaylistGetter,
                                        PlaylistSetter,
                                        PlaylistQuery,
                                        PlaylistDeleter,
                                        PlaylistEnumerator);

  target->Set(String::New("PlaylistContainer"), constructor_template->GetFunction());
}

int PlaylistContainer::num_playlists() {
  return sp_playlistcontainer_num_playlists(playlist_container_);
}
