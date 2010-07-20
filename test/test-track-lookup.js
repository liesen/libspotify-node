require('./test');

createSession(function (session) {
  var refs = 0;
  function decref() {
    if (--refs === 0)
      session.logout(assert.ifError);
  }

  var malformedLink = 'foo:bar';
  var albumLink = 'spotify:album:5UfXvVB6oMHgnuT25R5jAs';
  var trackLink = 'spotify:track:01gCUID7bHTcp6JzeTfpIe';

  // fail by exception with a malformed link
  assert.throws(function(){
    session.getTrackByLink(malformedLink);
  }, Error);

  // fail by callback error with a malformed link
  refs++;
  session.getTrackByLink(malformedLink, function(err) {
    assert.ok(err instanceof Error);
    decref();
  });

  // fail by exception with a non-track link
  assert.throws(function(){
    session.getTrackByLink(albumLink);
  }, Error);

  // fail by callback error with a non-track link
  refs++;
  session.getTrackByLink(albumLink, function(err) {
    assert.ok(err instanceof Error);
    decref();
  });

  // succeed with loading a track
  refs++;
  session.getTrackByLink(trackLink, function(err, track) {
    assert.ok(track instanceof spotify.Track);
    assert.strictEqual(track.loaded, true);
    decref();
  });

});
