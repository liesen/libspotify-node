require('./test');

createSession(function (session) {
  session.playlists.addListener('load', function () {
    //sys.print(sys.inspect(this));
    // this will fail if you don't have any playlists...
    assert.strictEqual(typeof this.length, "number");
    assert.ok(this.length > 0);
    session.logout(assert.ifError);
  })
});
