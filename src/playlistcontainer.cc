#include "playlistcontainer.h"
#include "playlist.h"

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> PlaylistContainer::constructor_template;

#define THROW_EXCEPTION(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

static inline char* ToCString(Handle<Value> value) {
  Local<String> str = value->ToString();
  char *p = new char[str->Utf8Length()];
  str->WriteUtf8(p);
  return p;
}

// -----------------------------------------------------------------------------
// libspotify callbacks

static void PlaylistAdded(sp_playlistcontainer *pc,
                          sp_playlist *playlist,
                          int position,
                          void *userdata)
{
  HandleScope scope;

  // this is called on the main thread
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);

  // First, trigger any queued callback from Create
  create_callback_entry_t *entry;
  TAILQ_FOREACH(entry, &p->create_callback_queue_, link) {
		if (entry->playlist == playlist) {
      Local<Value> argv[] = { (*Undefined()), (*Playlist::New(playlist)) };
      (*entry->callback)->Call(p->handle_, 2, argv);
      sp_playlist_release(entry->playlist);
      cb_destroy(entry->callback);
      TAILQ_REMOVE(&p->create_callback_queue_, entry, link);
      delete entry;
		}
	}

  Handle<Value> argv[] = { Playlist::New(playlist), Integer::New(position) };
  p->Emit(String::New("playlistAdded"), 2, argv);
}

static void PlaylistRemoved(sp_playlistcontainer *pc,
                            sp_playlist *playlist,
                            int old_position,
                            void *userdata)
{
  // this is called on the main thread
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = {
    Playlist::New(playlist),
    Integer::New(old_position)
  };
  p->Emit(String::New("playlistRemoved"), 2, argv);
}

static void PlaylistMoved(sp_playlistcontainer *pc,
                            sp_playlist *playlist,
                            int old_position,
                            int new_position,
                            void *userdata)
{
  // this is called on the main thread
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = {
    Playlist::New(playlist),
    Integer::New(old_position),
    Integer::New(new_position)
  };
  p->Emit(String::New("playlistMoved"), 3, argv);
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
  PlaylistMoved,
  PlaylistContainerLoaded
};

PlaylistContainer::PlaylistContainer(sp_playlistcontainer* playlist_container)
  : EventEmitter(), playlist_container_(playlist_container) {
  if (this->playlist_container_)
    sp_playlistcontainer_add_callbacks(this->playlist_container_, &callbacks, this);
  TAILQ_INIT(&create_callback_queue_);
}

PlaylistContainer::~PlaylistContainer() {
  // Clear the create_callback_queue_
  create_callback_entry_t *entry;
  TAILQ_FOREACH(entry, &create_callback_queue_, link) {
    Local<Value> argv[] = { Exception::Error(String::New("aborted")) };
    (*entry->callback)->Call(handle_, 1, argv);
    sp_playlist_release(entry->playlist);
    cb_destroy(entry->callback);
    delete entry;
	}
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
  return scope.Close(Integer::New(
    sp_playlistcontainer_num_playlists(pc->playlist_container_)));
}

Handle<Value> PlaylistContainer::PlaylistGetter(uint32_t index, const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  sp_playlist* playlist = sp_playlistcontainer_playlist(
      pc->playlist_container_, index);
  if (!playlist) return Undefined();
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
  return scope.Close(Boolean::New(
    index < sp_playlistcontainer_num_playlists(pc->playlist_container_)));
}

Handle<Array> PlaylistContainer::PlaylistEnumerator(const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int count = sp_playlistcontainer_num_playlists(pc->playlist_container_);
  Local<Array> playlists = Array::New(count);

  for (int i = 0; i < count; i++) {
    sp_playlist* playlist = sp_playlistcontainer_playlist(
        pc->playlist_container_, i);
    playlists->Set(i, Playlist::New(playlist));
  }

  return scope.Close(playlists);
}

Handle<Value> PlaylistContainer::Create(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 1)
    THROW_EXCEPTION(TypeError, "search takes at least 1 argument");
  if (!args[0]->IsString())
    THROW_EXCEPTION(TypeError, "first argument must be a string");
  if (args.Length() > 1 && !args[1]->IsFunction())
    THROW_EXCEPTION(TypeError, "last argument must be a function");

  PlaylistContainer* s = Unwrap<PlaylistContainer>(args.This());

  char *name = ToCString(args[0]);
  sp_playlist *playlist = sp_playlistcontainer_add_new_playlist(
    s->playlist_container_,
    //(*String::Utf8Value((args[0])->ToString()))
    name
  );
  delete name;

  if (!playlist)
    THROW_EXCEPTION(Error, "failed to create new playlist");

  // queue callback
  if (args.Length() > 1) {
    create_callback_entry_t *entry = new create_callback_entry_t;
    entry->playlist = playlist;
    sp_playlist_add_ref(entry->playlist);
    entry->callback = cb_persist(args[1]);
    TAILQ_INSERT_TAIL(&s->create_callback_queue_, entry, link);
  }

  return scope.Close(Playlist::New(playlist));
}

void PlaylistContainer::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("PlaylistContainer"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "create", Create);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(PlaylistGetter,
                                        PlaylistSetter,
                                        PlaylistQuery,
                                        PlaylistDeleter,
                                        PlaylistEnumerator);

  target->Set(String::New("PlaylistContainer"), constructor_template->GetFunction());
}
