#include <node.h>
#include <string.h>
#include <stdlib.h>

#include "magic.h"

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    Persistent<Function> callback;

    char* filepath;

    // libmagic handle
    struct magic_set *magic;

    bool error;
    const char* error_message;

    const char* result;
};

static Persistent<FunctionTemplate> constructor;

const char* ToCString(const String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

class Magic : public ObjectWrap {
  public:
    struct magic_set *magic;

    Magic(const char* magicfile, int mflags) {
      magic = magic_open(mflags);
      if (magic == NULL)
        ThrowException(Exception::Error(String::New(uv_strerror(uv_last_error(uv_default_loop())))));
      if (magic_load(magic, magicfile) == -1) {
        Local<String> errstr = String::New(magic_error(magic));
        magic_close(magic);
        magic = NULL;
        ThrowException(Exception::Error(errstr));
      }
    }
    ~Magic() {
      if (magic != NULL)
        magic_close(magic);
      magic = NULL;
    }

    static Handle<Value> New(const Arguments& args) {
      HandleScope scope;
      int mflags = MAGIC_SYMLINK;
      char* path = NULL;

      if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create instances of this object."))
        );
      }

      if (args.Length() > 0) {
        if (args[0]->IsString()) {
          String::Utf8Value str(args[0]->ToString());
          path = strdupa((const char*)(*str));
        } else if (args[0]->IsInt32())
          mflags = args[0]->Int32Value();
        else {
          return ThrowException(Exception::TypeError(
              String::New("First argument must be a string or integer")));
        }
      }
      if (args.Length() > 1) {
        if (args[1]->IsInt32())
          mflags = args[1]->Int32Value();
        else {
          return ThrowException(Exception::TypeError(
              String::New("Second argument must be an integer")));
        }
      }

      Magic* obj = new Magic(magic_getpath(path, 0/*FILE_LOAD*/), mflags);
      obj->Wrap(args.This());

      return args.This();
    }

    static Handle<Value> Detect(const Arguments& args) {
      HandleScope scope;
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a string")));
      }
      if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Second argument must be a callback function")));
      }

      Local<Function> callback = Local<Function>::Cast(args[1]);

      String::Utf8Value str(args[0]->ToString());

      Baton* baton = new Baton();
      baton->error = false;
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->filepath = strdup((const char*)*str);
      baton->magic = obj->magic;

      int status = uv_queue_work(uv_default_loop(), &baton->request,
                                 Magic::DetectWork, Magic::DetectAfter);
      assert(status == 0);

      return Undefined();
    }

    static void DetectWork(uv_work_t* req) {
      Baton* baton = static_cast<Baton*>(req->data);

      baton->result = magic_file(baton->magic, baton->filepath);

      if (baton->result == NULL) {
        baton->error = true;
        baton->error_message = magic_error(baton->magic);
      }
    }

    static void DetectAfter(uv_work_t* req) {
      HandleScope scope;
      Baton* baton = static_cast<Baton*>(req->data);

      if (baton->error) {
        Local<Value> err = Exception::Error(String::New(baton->error_message));

        const unsigned argc = 1;
        Local<Value> argv[argc] = { err };

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
          node::FatalException(try_catch);
      } else {
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
          Local<Value>::New(Null()),
          Local<Value>::New(String::New(baton->result))
        };

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
          node::FatalException(try_catch);
      }

      free(baton->filepath);
      baton->callback.Dispose();
      delete baton;
    }

    static void Initialize(Handle<Object> target) {
      HandleScope scope;

      Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
      Local<String> name = String::NewSymbol("Magic");

      constructor = Persistent<FunctionTemplate>::New(tpl);
      constructor->InstanceTemplate()->SetInternalFieldCount(1);
      constructor->SetClassName(name);

      NODE_SET_PROTOTYPE_METHOD(constructor, "detect", Detect);

      target->Set(name, constructor->GetFunction());
    }
};

extern "C" {
  void init(Handle<Object> target) {
    HandleScope scope;
    Magic::Initialize(target);
  }

  NODE_MODULE(magic, init);
}
