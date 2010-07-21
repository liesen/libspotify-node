# node-spotify

[libspotify](http://developer.spotify.com/en/libspotify/) bindings for [nodejs](http://nodejs.org/).

> **Warning:** This is software under early development. Many features are missing and the API might change at any time without notification.

## Building & installation

    node-waf configure build test
    sudo node-waf install

### Requirements

- [Nodejs](http://nodejs.org/) >= 0.1.100
- [libspotify](http://developer.spotify.com/en/libspotify/) == 0.0.4

## Example

A simple example where we log in, search for `"album:belle"`, dump the results to stdout and finally log out:

    var session = new Session({applicationKey: myAppkey});
    session.login("username", "password", function (err) {
      if (err) return sys.error(err.stack || err);
      session.search('album:belle', function(err, result){
        sys.puts(sys.inspect(result));
        session.logout();
      })
    });

Fore more examples, have a look in the `examples` directory.

## Documentation

See [doc/api.md](doc/api.md)

## MIT license

Copyright (c) 2010 Johan Liesén & Rasmus Andersson <http://hunch.se/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
