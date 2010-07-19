require('./test');
createSession(function (session) {
  session.logout(assert.ifError);
});
