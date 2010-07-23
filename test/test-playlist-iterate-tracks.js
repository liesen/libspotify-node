require('./test');


createSession(function (session) {
  session.addListener('metadataUpdated', function () { sys.log('metadataUpdated'); });
  session.playlists.addListener('load', function () {
    if (this.length > 0) {
      function f () {
        this.removeAllListeners();

        for (var i = 0; i < this.length; i++) {
          assert.ok(this[i] instanceof spotify.Track);
        }

        if (this.length > 0) {
          sys.puts(sys.inspect(this[0]));
        }

        session.logout(function() { console.log('Logged out'); });
      }

      this[0].addListener('updated', f); 
    }
  });
});
