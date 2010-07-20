#include "index.h"
#include "session.h"
#include "playlistcontainer.h"
#include "playlist.h"
#include "search.h"
#include "track.h"
#include "album.h"
#include "artist.h"

extern "C" void init(Handle<Object> target) {
  HandleScope scope;
  Session::Initialize(target);
  PlaylistContainer::Initialize(target);
  Playlist::Initialize(target);
  SearchResult::Initialize(target);
  Track::Initialize(target);
  Album::Initialize(target);
  Artist::Initialize(target);
  target->Set(String::NewSymbol("version"), String::New("0.1"));
  target->Set(String::NewSymbol("spotifyAPIVersion"), Integer::New(SPOTIFY_API_VERSION));
}
