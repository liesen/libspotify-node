var assert = require('assert');
var spotify = require('../spotify');
var test = require('./test').createTestSession;
var sys = require('sys');

test(function (session) {
  session.logout(assert.ifError);
});


