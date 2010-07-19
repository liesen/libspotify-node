require('./test');
/*
  This test loads all playlists in session.playlists and performs sanity checks
  along the way.
*/

createSession(true, function (session) {
  var addedPlaylistsCounter = 0,
      loadedPlaylistsCounter = 0,
      refcount = 1,
      progress = 0; // 1 for "load" event;

  function finalize() {
    assert.equal(loadedPlaylistsCounter, session.playlists.length);
    session.logout(assert.ifError);
  }
  
  session.playlists.addListener('playlistAdded', function(playlist, position){
    addedPlaylistsCounter++;
    refcount++;
    playlist.addListener('updated', function(){
      loadedPlaylistsCounter++;
      var p = Math.round((loadedPlaylistsCounter / session.playlists.length)*50);
      if (p !== progress) { progress = p; sys.print("|"); }
      assert.ok(addedPlaylistsCounter >= loadedPlaylistsCounter);
      if (--refcount === 0) finalize();
    })
    playlist.addListener('tracksAdded', function(count, position){
      playlist._testTracksAdded = count;
    })
  })
  
  session.playlists.addListener('load', function(){
    sys.puts('Loading '+this.length+' playlists...');
    sys.puts("|------------------------------------------------| 100%");
    assert.equal(addedPlaylistsCounter, this.length);
    if (--refcount === 0) finalize();
  })
});
