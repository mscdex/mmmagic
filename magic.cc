#include <node.h>
#include <node_buffer.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
# ifndef _delayimp_h 
   extern "C" IMAGE_DOS_HEADER __ImageBase; 
# endif 
#endif

#include "magic.h"

using namespace node;
using namespace v8;

struct Baton {
    uv_work_t request;
    Persistent<Function> callback;

    char* data;
    uint32_t dataLen;
    bool dataIsPath;

    // libmagic handle
    struct magic_set *magic;

    bool error;
    const char* error_message;

    const char* result;
};

static Persistent<FunctionTemplate> constructor;

class Magic : public ObjectWrap {
  public:
    struct magic_set *magic;

    Magic(const char* magicfile, int mflags) {
      HandleScope scope;
      if (magicfile != NULL) {
        /* Windows blows up trying to look up the path '(null)' returned by
           magic_getpath() */
        if (strncmp(magicfile, "(null)", 6) == 0)
          magicfile = NULL;
      }
      magic = magic_open(mflags);
      if (magic == NULL)
        ThrowException(Exception::Error(String::New(uv_strerror(uv_last_error(uv_default_loop())))));
      if (magic_load(magic, NULL) == -1) {
#ifdef _WIN32
        /* Use magic file contained in addon distribution as last resort */
        char addonPath[MAX_PATH];
        GetModuleFileName((HMODULE)&__ImageBase, addonPath, sizeof(addonPath));
        (strrchr(addonPath, '\\'))[0] = 0;
        (strrchr(addonPath, '\\'))[1] = 0;
        strcat(addonPath, "magic\\magic");
        if (magic_load(magic, addonPath) == -1) {
#endif
          Local<String> errstr = String::New(magic_error(magic));
          magic_close(magic);
          magic = NULL;
          ThrowException(Exception::Error(errstr));
#ifdef _WIN32
        }
#endif
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
            String::New("Use `new` to create instances of this object."))
        );
      }

      if (args.Length() > 0) {
        if (args[0]->IsString()) {
          String::Utf8Value str(args[0]->ToString());
          path = strdup((const char*)(*str));
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
          if (path)
            free(path);
          return ThrowException(Exception::TypeError(
              String::New("Second argument must be an integer")));
        }
      }

      Magic* obj = new Magic(magic_getpath(path, 0/*FILE_LOAD*/), mflags);
      obj->Wrap(args.This());

      if (path)
        free(path);

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
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->data = strdup((const char*)*str);
      baton->dataIsPath = true;
      baton->magic = obj->magic;

      int status = uv_queue_work(uv_default_loop(), &baton->request,
                                 Magic::DetectWork, Magic::DetectAfter);
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
      Local<Object> buffer_obj = args[0]->ToObject();

      Baton* baton = new Baton();
      baton->error = false;
      baton->request.data = baton;
      baton->callback = Persistent<Function>::New(callback);
      baton->data = Buffer::Data(buffer_obj);
      baton->dataLen = Buffer::Length(buffer_obj);
      baton->dataIsPath = false;
      baton->magic = obj->magic;

      int status = uv_queue_work(uv_default_loop(), &baton->request,
                                 Magic::DetectWork, Magic::DetectAfter);
      assert(status == 0);

      return Undefined();
    }

    static void DetectWork(uv_work_t* req) {
      Baton* baton = static_cast<Baton*>(req->data);

      if (baton->dataIsPath)
        baton->result = magic_file(baton->magic, baton->data);
      else {
        baton->result = magic_buffer(baton->magic, (const void*)baton->data,
                                     baton->dataLen);
      }

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

      if (baton->dataIsPath)
        free(baton->data);
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

      NODE_SET_PROTOTYPE_METHOD(constructor, "detectFile", DetectFile);
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
