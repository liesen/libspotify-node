#ifndef SPOTIFY_PLAYLISTCONTAINER_H_
#define SPOTIFY_PLAYLISTCONTAINER_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

namespace spotify {

class PlaylistContainer : public EventEmitter {
 public:
  static Handle<Object> New(sp_playlistcontainer *playlist_container) {
    Handle<ObjectTemplate> t = ObjectTemplate::New();
    t->SetInternalFieldCount(1);

    sp_playlistcontainer_callbacks callbacks = {
      PlaylistAdded,
      PlaylistRemoved,
      NULL,
      NULL
    };

    sp_playlistcontainer_add_callbacks(playlist_container, &callbacks, NULL);

    // Properties
    t->SetIndexedPropertyHandler(PlaylistGetter,
        											 	 PlaylistSetter,
        											 	 PlaylistQuery,
        											 	 PlaylistDeleter,
        											 	 PlaylistEnumerator);
    
    // Accessors
    t->SetAccessor(String::New("length"), NumPlaylists);
    Local<Object> instance = t->NewInstance();
    instance->SetInternalField(0, External::New(playlist_container));
    return instance;
  }

  protected:
    static Handle<Value> NumPlaylists(Local<String> property,
                                      const AccessorInfo& info) {
			HandleScope scope;
      sp_playlistcontainer* playlist_container = Unwrap(info.Holder());
      int num_playlists = sp_playlistcontainer_num_playlists(playlist_container);
      return scope.Close(Integer::New(num_playlists));
    }

    static Handle<Value> PlaylistGetter(uint32_t index, 
                                        const AccessorInfo& info) {
			HandleScope scope;
      sp_playlistcontainer* pc = Unwrap(info.Holder());
      sp_playlist* playlist = sp_playlistcontainer_playlist(pc, index);
      const char* playlist_name = sp_playlist_name(playlist);
      return scope.Close(String::New(playlist_name));
    }

		static Handle<Value> PlaylistSetter(uint32_t index,
																			  Local<Value> value,
         															 	const AccessorInfo& info) {
			return Handle<Value>();
		}

		static Handle<Boolean> PlaylistDeleter(uint32_t index,
         															 	   const AccessorInfo& info) {
			return Handle<Boolean>();
		}


		static Handle<Boolean> PlaylistQuery(uint32_t index,
																				 const AccessorInfo& info) {
			HandleScope scope;
			sp_playlistcontainer *pc = Unwrap(info.Holder());
			int num_playlists = sp_playlistcontainer_num_playlists(pc);
			return scope.Close(Boolean::New(index < num_playlists));
		}

    static Handle<Array> PlaylistEnumerator(const AccessorInfo& info) {
			HandleScope scope;
      sp_playlistcontainer *pc = Unwrap(info.Holder());
      int num_playlists = sp_playlistcontainer_num_playlists(pc);
      fprintf(stderr, "sp: playlist enumerator, %d playlists\n", num_playlists);
      return scope.Close(Array::New(num_playlists));
    }

    static void PlaylistAdded(sp_playlistcontainer *pc,
                              sp_playlist *playlist,
                              int position,
                              void *userdata) {
      fprintf(stderr, "sp: playlist added %d\n", position);
			// Emit("playlist_added", 0, NULL);
    }

    static void PlaylistRemoved(sp_playlistcontainer *pc,
    										        sp_playlist *playlist,
          											int position,
          											void *userdata) {
      fprintf(stderr, "sp: playlist removed%d\n", position);
    }

  private:
    static sp_playlistcontainer* Unwrap(const Handle<Object> self) {
      return node::ObjectWrap::Unwrap<sp_playlistcontainer>(self);
    }
};

}

#endif
