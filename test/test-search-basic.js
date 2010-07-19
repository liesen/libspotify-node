require('./test');

createSession(function (session) {
  var query = 'belle'; // avoid url reserved chars since we test the uri
  session.search(query, function(err, result){
    assert.ifError(err);
    //sys.puts(sys.inspect(result)) // warning: it's massive

    assert.strictEqual(typeof result.totalTracks, "number");
    assert.strictEqual(result.query, query);
    assert.strictEqual(typeof result.uri, "string");
    assert.strictEqual(result.uri, "spotify:search:"+query);
    assert.ok(Array.isArray(result.tracks));
    assert.ok(Array.isArray(result.albums));
    assert.ok(Array.isArray(result.artists));

    session.logout(assert.ifError);
  })
});
