var assert = require('assert');
var spotify = require('../spotify');
var sys = require('sys');

exports.createSession = function(account, thunk) {
  var session = new spotify.Session({applicationKey: account.applicationKey});
  session.addListener('logMessage', sys.log);
  session.login(account.username, account.password, function (err) {
    assert.ifError(err);
    thunk(session);
  });
}

exports.createTestSession = function(thunk) { 
  var account = require('./account');
  exports.createSession(account, thunk)
}
