#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
# include <wchar.h>
#endif

#include "magic.h"

using namespace node;
using namespace v8;

struct Baton {
  uv_work_t request;
  Nan::Callback* callback;

  char* data;
  uint32_t dataLen;
  bool dataIsPath;

  // libmagic info
  const char* path;
  int flags;

  bool error;
  bool free_error;
  char* error_message;

  const char* result;
};

static Nan::Persistent<Function> constructor;
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

    static void New(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Nan::HandleScope();
#ifndef _WIN32
      int mflags = MAGIC_SYMLINK;
#else
      int mflags = MAGIC_NONE;
#endif
      char* path = NULL;
      bool use_bundled = true;

      if (!args.IsConstructCall())
        return Nan::ThrowTypeError("Use `new` to create instances of this object.");

      if (args.Length() > 1) {
        if (args[1]->IsInt32())
          mflags = args[1]->Int32Value();
        else
          return Nan::ThrowTypeError("Second argument must be an integer");
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
        } else
          return Nan::ThrowTypeError("First argument must be a string or integer");
      }

      Magic* obj = new Magic((use_bundled ? NULL : path), mflags);
      obj->Wrap(args.This());
      obj->Ref();

      return args.GetReturnValue().Set(args.This());
    }

    static void DetectFile(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Nan::HandleScope();
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (!args[0]->IsString())
        return Nan::ThrowTypeError("First argument must be a string");
      if (!args[1]->IsFunction())
        return Nan::ThrowTypeError("Second argument must be a callback function");

      Local<Function> callback = Local<Function>::Cast(args[1]);

      String::Utf8Value str(args[0]->ToString());

      Baton* baton = new Baton();
      baton->error = false;
      baton->free_error = true;
      baton->error_message = NULL;
      baton->request.data = baton;
      baton->callback = new Nan::Callback(callback);
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

      args.GetReturnValue().Set(Nan::Undefined());
    }

    static void Detect(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Nan::HandleScope();
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (args.Length() < 2)
        return Nan::ThrowTypeError("Expecting 2 arguments");
      if (!Buffer::HasInstance(args[0]))
        return Nan::ThrowTypeError("First argument must be a Buffer");
      if (!args[1]->IsFunction())
        return Nan::ThrowTypeError("Second argument must be a callback function");

      Local<Function> callback = Local<Function>::Cast(args[1]);
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 10
      Local<Object> buffer_obj = args[0]->ToObject();
#else
      Local<Value> buffer_obj = args[0];
#endif

      Baton* baton = new Baton();
      baton->error = false;
      baton->free_error = true;
      baton->error_message = NULL;
      baton->request.data = baton;
      baton->callback = new Nan::Callback(callback);
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

      return args.GetReturnValue().Set(args.This());
    }

    static void DetectWork(uv_work_t* req) {
      Baton* baton = static_cast<Baton*>(req->data);
      const char* result;
      struct magic_set *magic = magic_open(baton->flags
                                           | MAGIC_NO_CHECK_COMPRESS
                                           | MAGIC_ERROR);

      if (magic == NULL) {
#if NODE_MODULE_VERSION <= 0x000B
        baton->error_message = strdup(uv_strerror(
                                        uv_last_error(uv_default_loop())));
#else
// XXX libuv 1.x currently has no public cross-platform function to convert an
//     OS-specific error number to a libuv error number. `-errno` should work
//     for *nix, but just passing GetLastError() on Windows will not work ...
# ifdef _MSC_VER
        baton->error_message = strdup(uv_strerror(GetLastError()));
# else
        baton->error_message = strdup(uv_strerror(-errno));
# endif
#endif
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

      if (baton->dataIsPath) {
#ifdef _WIN32
        // open the file manually to help cope with potential unicode characters
        // in filename
        const char* ofn = baton->data;
        int flags = O_RDONLY|O_BINARY;
        int fd = -1;
        int wLen;
        wLen = MultiByteToWideChar(CP_UTF8, 0, ofn, -1, NULL, 0);
        if (wLen > 0) {
          wchar_t* wfn = (wchar_t*)malloc(wLen * sizeof(wchar_t));
          if (wfn) {
            int wret = MultiByteToWideChar(CP_UTF8, 0, ofn, -1, wfn, wLen);
            if (wret != 0)
              fd = _wopen(wfn, flags);
            free(wfn);
            wfn = NULL;
          }
        }
        if (fd == -1) {
          baton->error = true;
          baton->free_error = false;
          baton->error_message = "Error while opening file";
          magic_close(magic);
          return;
        }
        result = magic_descriptor(magic, fd);
        _close(fd);
#else
        result = magic_file(magic, baton->data);
#endif
      } else
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
      Nan::HandleScope scope;
      Baton* baton = static_cast<Baton*>(req->data);

      if (baton->error) {
        Local<Value> err = Nan::Error(baton->error_message);

        if (baton->free_error)
          free(baton->error_message);

        Local<Value> argv[1] = { err };
        baton->callback->Call(1, argv);
      } else {
        Local<Value> argv[2] = {
          Nan::Null(),
          Local<Value>(baton->result
                       ? Nan::New<String>(baton->result).ToLocalChecked()
                       : Nan::New<String>().ToLocalChecked())
        };

        if (baton->result)
          free((void*)baton->result);

        baton->callback->Call(2, argv);
      }

      if (baton->dataIsPath)
        free(baton->data);
      delete baton->callback;
      delete baton;
    }

    static void SetFallback(const Nan::FunctionCallbackInfo<v8::Value>& args) {

      if (fallbackPath)
        free((void*)fallbackPath);

      if (args.Length() > 0 && args[0]->IsString()
          && args[0]->ToString()->Length() > 0) {
        String::Utf8Value str(args[0]->ToString());
        fallbackPath = strdup((const char*)(*str));
      } else
        fallbackPath = NULL;

      return args.GetReturnValue().Set(args.This());
    }

    static void Initialize(Handle<Object> target) {

      Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(Nan::New<String>("Magic").ToLocalChecked());
      Nan::SetPrototypeMethod(tpl, "detectFile", DetectFile);
      Nan::SetPrototypeMethod(tpl, "detect", Detect);

      constructor.Reset(tpl->GetFunction());
      target->Set(Nan::New<String>("setFallback").ToLocalChecked(),
                  Nan::New<FunctionTemplate>(SetFallback)->GetFunction());

      target->Set(Nan::New<String>("Magic").ToLocalChecked(), tpl->GetFunction());
    }
};

extern "C" {
  void init(Handle<Object> target) {
    Nan::HandleScope();
    Magic::Initialize(target);
  }

  NODE_MODULE(magic, init);
}
