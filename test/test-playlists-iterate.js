require('./test');

createSession(function (session) {
  session.playlists.addListener('load', function () {
    var playlists = [];

    for (var i = 0; i < this.length; i++) {
      assert.ok(this[i] instanceof spotify.Playlist);
      playlists[i] = this[i];
    }

    for (var i = 0; i < this.length; i++) {
      assert.ok(this[i] instanceof spotify.Playlist);
      assert.strictEqual(this[i], playlists[i]);
    }

    session.logout(assert.ifError);
  })
});
