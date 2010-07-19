var assert = require('assert');
var spotify = require('../spotify');
var test = require('./test').createTestSession;
var sys = require('sys');

test(function (session) {
  session.playlists.addListener('load', function () {
    for (var i = 0; i < this.length; i++) {
      assert.ok(this[i] instanceof spotify.Playlist);
    }

    session.logout(assert.ifError);
  })
});
