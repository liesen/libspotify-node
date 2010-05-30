#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <v8.h>
#include <node.h>
#include <libspotify/api.h>

#include "appkey.c"
#include "spotify.h"

using namespace std;
using namespace v8;

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;

int g_exit_code = -1;
static pthread_t g_main_thread = (pthread_t) -1;

static void notify_main_thread(sp_session *session) {
  // fprintf(stderr, "(notify main thread)\n");
  pthread_kill(g_main_thread, SIGIO);
}

static void sigIgn(int signo) {
}




static void Log(const char* event) {
  printf("Logged: %s\n", event);
}

static Handle<Value> LogCallback(const Arguments& args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope;
  Handle<Value> arg = args[0];
  String::Utf8Value value(arg);
  Log(*value);
  return Undefined();
}


Handle<String> ReadFile(const string& name) {
  FILE* file = fopen(name.c_str(), "rb");
  if (file == NULL) return Handle<String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  Handle<String> result = String::New(chars, size);
  delete[] chars;
  return result;
}

bool ExecuteScript(Handle<String> script) {
  HandleScope handle_scope;

  // We're just about to compile the script; set up an error handler to
  // catch any exceptions the script might throw.
  TryCatch try_catch;

  // Compile the script and check for errors.
  Handle<Script> compiled_script = Script::Compile(script);
  if (compiled_script.IsEmpty()) {
    String::Utf8Value error(try_catch.Exception());
    Log(*error);
    // The script failed to compile; bail out.
    return false;
  }

  // Run the script!
  Handle<Value> result = compiled_script->Run();
  if (result.IsEmpty()) {
    // The TryCatch above is still in effect and will have caught the error.
    String::Utf8Value error(try_catch.Exception());
    Log(*error);
    // Running the script failed; bail out.
    return false;
  }
  return true;
}


int main(int argc, char* argv[]) {
  string file(argv[1]);
  fprintf(stderr, "File '%s'\n", file.c_str());

  HandleScope scope;
  
  Local<ObjectTemplate> global = ObjectTemplate::New();
  global->Set(String::NewSymbol("log"), FunctionTemplate::New(LogCallback));

  Persistent<Context> context = Context::New(NULL, global);
  Context::Scope context_scope(context);

  Local<ObjectTemplate> t = ObjectTemplate::New();
  Local<Object> instance = t->NewInstance();
  spotify::Spotify::Initialize(context->Global());

  Handle<String> source = ReadFile(file);
  Local<Script> script = Script::Compile(source);

  Local<Value> result = script->Run();
  context.Dispose();

  String::AsciiValue ascii(result);
  printf("%s\n", *ascii);

  V8::Dispose();
  return 0;
}

