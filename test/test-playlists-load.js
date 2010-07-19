require('./test');

createSession(function (session) {
  session.playlists.addListener('load', function () {
    sys.print(sys.inspect(this));
    session.logout(assert.ifError);
  })
});
