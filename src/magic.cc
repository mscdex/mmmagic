#include <node.h>
#include <node_buffer.h>
#include <string.h>
#include <stdlib.h>

#include "magic.h"

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    Persistent<Function> callback;

    char* data;
    uint32_t dataLen;
    bool dataIsPath;

    // libmagic info
    const char* path;
    int flags;

    bool error;
    char* error_message;

    const char* result;
};

static Persistent<FunctionTemplate> constructor;
static const char* fallbackPath;

class Magic : public ObjectWrap {
  public:
    const char* mpath;
    int mflags;

    Magic(const char* path, int flags) {
      if (path != NULL) {
        /* Windows blows up trying to look up the path '(null)' returned by
           magic_getpath() */
        if (strncmp(path, "(null)", 6) == 0)
          path = NULL;
      }
      mpath = (path == NULL ? strdup(fallbackPath) : path);
      mflags = flags;
    }
    ~Magic() {
      if (mpath != NULL) {
        free((void*)mpath);
        mpath = NULL;
      }
    }

    static Handle<Value> New(const Arguments& args) {
      HandleScope scope;
#ifndef _WIN32
      int mflags = MAGIC_SYMLINK;
#else
      int mflags = MAGIC_NONE;
#endif
      char* path = NULL;
      bool use_bundled = true;

      if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use `new` to create instances of this object."))
        );
      }

      if (args.Length() > 1) {
        if (args[1]->IsInt32())
          mflags = args[1]->Int32Value();
        else {
          return ThrowException(Exception::TypeError(
              String::New("Second argument must be an integer")));
        }
      }

      if (args.Length() > 0) {
        if (args[0]->IsString()) {
          use_bundled = false;
          String::Utf8Value str(args[0]->ToString());
          path = strdup((const char*)(*str));
        } else if (args[0]->IsInt32())
          mflags = args[0]->Int32Value();
        else if (args[0]->IsBoolean() && !args[0]->BooleanValue()) {
          use_bundled = false;
          path = strdup(magic_getpath(NULL, 0/*FILE_LOAD*/));
        } else {
          return ThrowException(Exception::TypeError(
              String::New("First argument must be a string or integer")));
        }
      }

      Magic* obj = new Magic((use_bundled ? NULL : path), mflags);
      obj->Wrap(args.This());
      obj->Ref();

      return args.This();
    }

    static Handle<Value> DetectFile(const Arguments& args) {
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
      baton->error_message = NULL;
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->data = strdup((const char*)*str);
      baton->dataIsPath = true;
      baton->path = obj->mpath;
      baton->flags = obj->mflags;
      baton->result = NULL;

      int status = uv_queue_work(uv_default_loop(),
                                 &baton->request,
                                 Magic::DetectWork,
                                 (uv_after_work_cb)Magic::DetectAfter);
      assert(status == 0);

      return Undefined();
    }

    static Handle<Value> Detect(const Arguments& args) {
      HandleScope scope;
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (args.Length() < 2) {
        return ThrowException(Exception::TypeError(
            String::New("Expecting 2 arguments")));
      }
      if (!Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a Buffer")));
      }
      if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Second argument must be a callback function")));
      }

      Local<Function> callback = Local<Function>::Cast(args[1]);
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 10
      Local<Object> buffer_obj = args[0]->ToObject();
#else
      Local<Value> buffer_obj = args[0];
#endif

      Baton* baton = new Baton();
      baton->error = false;
      baton->error_message = NULL;
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->data = Buffer::Data(buffer_obj);
      baton->dataLen = Buffer::Length(buffer_obj);
      baton->dataIsPath = false;
      baton->path = obj->mpath;
      baton->flags = obj->mflags;
      baton->result = NULL;

      int status = uv_queue_work(uv_default_loop(),
                                 &baton->request,
                                 Magic::DetectWork,
                                 (uv_after_work_cb)Magic::DetectAfter);
      assert(status == 0);

      return Undefined();
    }

    static void DetectWork(uv_work_t* req) {
      Baton* baton = static_cast<Baton*>(req->data);
      const char* result;
      struct magic_set *magic = magic_open(baton->flags | MAGIC_NO_CHECK_COMPRESS);

      if (magic == NULL) {
        baton->error_message = strdup(uv_strerror(
                                        uv_last_error(uv_default_loop())));
      } else if (magic_load(magic, baton->path) == -1
                 && magic_load(magic, fallbackPath) == -1) {
        baton->error_message = strdup(magic_error(magic));
        magic_close(magic);
        magic = NULL;
      }

      if (magic == NULL) {
        if (baton->error_message)
          baton->error = true;
        return;
      }

      if (baton->dataIsPath)
        result = magic_file(magic, baton->data);
      else
        result = magic_buffer(magic, (const void*)baton->data, baton->dataLen);

      if (result == NULL) {
        const char* error = magic_error(magic);
        if (error) {
          baton->error = true;
          baton->error_message = strdup(error);
        }
      } else
        baton->result = strdup(result);

      magic_close(magic);
    }

    static void DetectAfter(uv_work_t* req) {
      HandleScope scope;
      Baton* baton = static_cast<Baton*>(req->data);

      if (baton->error) {
        Local<Value> err = Exception::Error(String::New(baton->error_message));
        free(baton->error_message);

        const unsigned argc = 1;
        Local<Value> argv[argc] = { err };

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
          FatalException(try_catch);
      } else {
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
          Local<Value>::New(Null()),
          Local<Value>::New(baton->result
                            ? String::New(baton->result)
                            : String::Empty())
        };

        if (baton->result)
          free((void*)baton->result);

        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught())
          FatalException(try_catch);
      }

      if (baton->dataIsPath)
        free(baton->data);
      baton->callback.Dispose();
      delete baton;
    }

    static Handle<Value> SetFallback(const Arguments& args) {
      HandleScope scope;

      if (fallbackPath)
        free((void*)fallbackPath);

      if (args.Length() > 0 && args[0]->IsString()
          && args[0]->ToString()->Length() > 0) {
        String::Utf8Value str(args[0]->ToString());
        fallbackPath = strdup((const char*)(*str));
      } else
        fallbackPath = NULL;

      return Undefined();
    }

    static void Initialize(Handle<Object> target) {
      HandleScope scope;

      Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
      Local<String> name = String::NewSymbol("Magic");

      constructor = Persistent<FunctionTemplate>::New(tpl);
      constructor->InstanceTemplate()->SetInternalFieldCount(1);
      constructor->SetClassName(name);

      NODE_SET_PROTOTYPE_METHOD(constructor, "detectFile", DetectFile);
      NODE_SET_PROTOTYPE_METHOD(constructor, "detect", Detect);
      target->Set(String::NewSymbol("setFallback"),
        FunctionTemplate::New(SetFallback)->GetFunction());

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
