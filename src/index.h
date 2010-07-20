// Common includes, constants and helpers.
// ----------------------------------------------------------------------------
#ifndef SPOTIFY_INDEX_H_
#define SPOTIFY_INDEX_H_

// We almost always need access to these
#include <node.h>           // includes v8.h, ev.h, eio.h, sys/types.h, etc
#include <node_events.h>    // EventEmitter
#include <libspotify/api.h> // Spotify API

// Since we are not building a C++ API, we dont' care about namespaces in .h's
using namespace v8;
using namespace node;

#define STRPTR(obj) (*String::Utf8Value((obj)->ToString()))
#define STRSIZ(obj) ((obj)->ToString()->Utf8Length())

// ----------------------------------------------------------------------------
// Helpers

// Throwing exceptions
#define JS_THROW(t, s) ThrowException(Exception::t(String::New(s)))
#define JS_THROWF(tmpl, ...)\
  { char msg[1024]; snprintf(msg,1023,tmpl,__VA_ARGS__); JS_THROW(msg); }

// Creates a UTF-8 C string from a Value.
// Note: if you only need to access the string (i.e. not make a copy of it) you
// can use String::Utf8Value:
//   String::Utf8Value foo(value);
//   const char *temp = *foo;
static inline char* ToCString(Handle<Value> value) {
  Local<String> str = value->ToString();
  char *p = new char[str->Utf8Length()];
  str->WriteUtf8(p);
  return p;
}

#endif // SPOTIFY_INDEX_H_
