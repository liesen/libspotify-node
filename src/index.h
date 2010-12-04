// Common includes, constants and helpers.
// ----------------------------------------------------------------------------
#ifndef SPOTIFY_INDEX_H_
#define SPOTIFY_INDEX_H_

#include <node.h>  // includes v8.h, ev.h, eio.h, sys/types.h, etc
#include <node_events.h>  // EventEmitter
#include <libspotify/api.h>  // Spotify API

using namespace v8;
using namespace node;


enum MetadataUpdateType {
  kMetadataUpdateTypeTrack = 0,
  kMetadataUpdateTypeAlbum = 1,
  kMetadataUpdateTypeArtist = 2,
};

// Property getter interface boilerplate
#define GETTER_H(name)\
  static Handle<Value> name(Local<String> property, const AccessorInfo& info)

// Property getter implementation boilerplate
#define GETTER_C(name)\
  Handle<Value> name(Local<String> property, const AccessorInfo& info)

// -----------------------------------------------------------------------------
// Helpers

// Emitting WIP/development notes
#define TODO(tmpl, ...)\
  fprintf(stderr, "TODO [node-spotify %s:%d] " tmpl "\n", \
          __FILE__, __LINE__, ##__VA_ARGS__)

// Dump a message to stderr
#define DPRINTF(tmpl, ...)\
  do {\
    fprintf(stderr, "D [node-spotify %s:%d] " tmpl "\n", \
            __FILE__, __LINE__, ##__VA_ARGS__);\
    fflush(stderr);\
  } while (0)

// Throwing exceptions
#define JS_THROW(t, s) ThrowException(Exception::t(String::New(s)))
#define JS_THROWF(tmpl, ...) {\
  char msg[1024];\
  snprintf(msg, sizeof(msg), tmpl, __VA_ARGS__);\
  JS_THROW(msg);\
}

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

// -----------------------------------------------------------------------------
// Invoking a callback with an error

inline Handle<Value> CallbackError(const Handle<Object> &context,
                                   const Handle<Value> &callback,
                                   const Handle<Value> &exc) {
  Handle<Value> argv[] = { exc };
  Function::Cast(*callback)->Call(context, 1, argv);
  return Undefined();
}

inline Handle<Value> CallbackError(const Handle<Object> &context,
                                   const Handle<Value> &callback,
                                   const char *message) {
  return CallbackError(context, callback,
                       Exception::Error(String::New(message)) );
}

inline Handle<Value> CallbackError(const Handle<Object> &context,
                                   const Handle<Value> &callback,
                                   sp_error errcode) {
  return CallbackError(context, callback,
    Exception::Error(String::New(sp_error_message(errcode))) );
}

// -----------------------------------------------------------------------------
// Invoking callback with an error or throwing an exception

inline Handle<Value> CallbackOrThrowError(const Handle<Object> &context,
                                          const Handle<Value> &callback,
                                          const Handle<Value> &exc) {
  if (callback.IsEmpty() || !callback->IsFunction()) {
    return ThrowException(exc);
  } else {
    return CallbackError(context, callback, exc);
  }
}

inline Handle<Value> CallbackOrThrowError(const Handle<Object> &context,
                                          const Handle<Value> &callback,
                                          const char *utf8str) {
  return CallbackOrThrowError(context, callback,
    Exception::Error(String::New(utf8str)));
}

inline Handle<Value> CallbackOrThrowError(const Handle<Object> &context,
                                          const Handle<Value> &callback,
                                          sp_error errcode) {
  return CallbackOrThrowError(context, callback, sp_error_message(errcode));
}

// -----------------------------------------------------------------------------
#endif // SPOTIFY_INDEX_H_
