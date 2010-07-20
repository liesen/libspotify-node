#include "playlist.h"

Persistent<FunctionTemplate> Playlist::constructor_template;

// -----------------------------------------------------------------------------
// internal helpers

static const char *_PlaylistURI(sp_playlist *pl) {
  static char uri_buf[256];
  sp_link *link = sp_link_create_from_playlist(pl);
  if (!link) return "?";
  sp_link_as_string(link, uri_buf, sizeof(uri_buf));
  sp_link_release(link);
  return uri_buf;
}

// -----------------------------------------------------------------------------
// libspotify callbacks

static void TracksAdded(sp_playlist *playlist, sp_track *const *tracks, int count, int position, void *userdata) {
  // called on the main thread
  //printf("[%s] tracksAdded(<tracks*>, %d, %d)\n", _PlaylistURI(playlist), count, position);
  Playlist* pl = static_cast<Playlist*>(userdata);
  Handle<Value> argv[] = {
    Integer::New(count),
    Integer::New(position)
    // todo: pass some kind of "invoke to create tracks" function?
  };
  pl->Emit(String::New("tracksAdded"), 2, argv);
}

static void TracksRemoved(sp_playlist *playlist, const int *tracks, int count, void *userdata) {
  // called on the main thread
  //printf("[%s] tracksRemoved(<track-indices*>, %d)\n", _PlaylistURI(playlist), count);
  Playlist* pl = static_cast<Playlist*>(userdata);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Integer::New(tracks[i]));
  }
  Handle<Value> argv[] = { array };
  pl->Emit(String::New("tracksRemoved"), 1, argv);
}

static void TracksMoved(sp_playlist *playlist, const int *tracks, int count, int new_position, void *userdata) {
  // called on the main thread
  //printf("[%s] tracksMoved(<track-indices*>, %d, %d)\n", _PlaylistURI(playlist), count, new_position);
  Playlist* pl = static_cast<Playlist*>(userdata);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Integer::New(tracks[i]));
  }
  Handle<Value> argv[] = { array, Integer::New(new_position) };
  pl->Emit(String::New("tracksMoved"), 2, argv);
}

static void PlaylistRenamed(sp_playlist *playlist, void *userdata) {
  // called on the main thread
  //printf("[%s] renamed -- name: %s\n", _PlaylistURI(playlist), sp_playlist_name(pl));
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("renamed"), 0, NULL);
}

static void PlaylistStateChanged(sp_playlist *playlist, void *userdata) {
  // called on the main thread
  // The "state" in this case are the flags like collaborative or pending.
  //printf("[%s] stateChanged -- colab: %d, pending: %d\n", _PlaylistURI(playlist),
  //  sp_playlist_is_collaborative(playlist), sp_playlist_has_pending_changes(playlist));
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("stateChanged"), 0, NULL);
}

static void PlaylistUpdateInProgress(sp_playlist *playlist, bool done, void *userdata) {
  // called on the main thread
  //printf("[%s] updateInProgress -- done: %s\n",
  //  _PlaylistURI(playlist), done ? "true" : "false");
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New(done ? "updated" : "updating"), 0, NULL);
}

static void PlaylistMetadataUpdated(sp_playlist *playlist, void *userdata) {
  // Called when metadata for one or more tracks in a playlist has been updated.
  //printf("[%s] metadataUpdated\n", _PlaylistURI(playlist));
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("metadataUpdated"), 0, NULL);
}

static sp_playlist_callbacks callbacks = {
  /* tracks_added */ TracksAdded,
  /* tracks_removed */ TracksRemoved,
  /* tracks_moved */ TracksMoved,
  /* playlist_renamed */ PlaylistRenamed,
  /* playlist_state_changed */ PlaylistStateChanged,
  /* playlist_update_in_progress */ PlaylistUpdateInProgress,
  /* playlist_metadata_updated */ PlaylistMetadataUpdated,
};

// -----------------------------------------------------------------------------
// Playlist implementation

Playlist::Playlist(sp_playlist* playlist)
  : node::EventEmitter()
  , playlist_(playlist)
{
  if (playlist_) {
    sp_playlist_add_ref(playlist_);
    sp_playlist_add_callbacks(playlist_, &callbacks, this);
  }
}

Playlist::~Playlist() {
  if (playlist_) sp_playlist_release(playlist_);
}

Handle<Value> Playlist::New(sp_playlist *playlist) {
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  Playlist *pl = ObjectWrap::Unwrap<Playlist>(instance);
  if (pl->playlist_) sp_playlist_release(pl->playlist_);
  pl->playlist_ = playlist;
  if (pl->playlist_) {
    sp_playlist_add_ref(pl->playlist_);
    sp_playlist_add_callbacks(pl->playlist_, &callbacks, pl);
  }
  return instance;
}

Handle<Value> Playlist::New(const Arguments& args) {
  (new Playlist(NULL))->Wrap(args.This());
  return args.This();
}

Handle<Value> Playlist::LoadedGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  if (!playlist) return Undefined();
  return scope.Close(Boolean::New(sp_playlist_is_loaded(playlist)));
}

Handle<Value> Playlist::NameGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  if (!playlist || !sp_playlist_is_loaded(playlist))
    return Undefined();
  return scope.Close(String::New(sp_playlist_name(playlist)));
}

Handle<Value> Playlist::URIGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Playlist *p = Unwrap<Playlist>(info.This());
  if (!p->playlist_ || !sp_playlist_is_loaded(p->playlist_))
    return Undefined();
  char buf[500]; // probably enough
  sp_link *link = sp_link_create_from_playlist(p->playlist_);
  if (!link)
    return Undefined();
  sp_link_as_string(link, buf, sizeof(buf));
  sp_link_release(link);
  return scope.Close(String::New(buf));
}

void Playlist::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Playlist"));
  constructor_template->Inherit(EventEmitter::constructor_template);
  
  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("loaded"), LoadedGetter);
  instance_t->SetAccessor(String::NewSymbol("name"), NameGetter);
  instance_t->SetAccessor(String::NewSymbol("uri"), URIGetter);
  /*instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(TrackGetter,
                                        TrackSetter,
                                        TrackQuery,
                                        TrackDeleter,
                                        TrackEnumerator);*/

  target->Set(String::New("Playlist"), constructor_template->GetFunction());
}
