require('./test');

createSession(function (session) {
  session.playlists.addListener('load', function () {
    for (var i = 0; i < this.length; i++) {
      assert.ok(this[i] instanceof spotify.Playlist);
    }
    session.logout(assert.ifError);
  })
});
