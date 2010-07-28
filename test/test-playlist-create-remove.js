require('./test');

createSession(function (session) {
  var playlistName = 'Testing: 1, 2, 3...';

  session.playlists.create(playlistName, function (playlist) {
    assert.equal(playlist.name, playlistName);
    var playlistUri = playlist.uri;
    
    // Outputting uri so that its removal can be verified
    console.log(playlistUri);

    session.on('playlistRemoved', function (playlist, oldPosition) {
      assert.equal(playlist.name, playlistName);
      assert.equal(playlist.uri, playlistUri);
      session.logout(assert.ifError);
    });

    // Adding "Dolly" by The Vanities
    var dolly = 'spotify:track:7lFuOq2cX3Y1G8tlDdLfAq';

    session.getTrackByLink(dolly, function (err, track) {
      assert.ifError(err);
      assert.equal(playlist.length, 0);
      playlist.push(session, track);
      assert.equal(playlist.length, 1);
      assert.equal(playlist[0].uri, dolly);
      session.playlists.remove(playlist);
    });
  });
});
