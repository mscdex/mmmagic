#include <errno.h>
#include <node.h>
#include <node_buffer.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
# include <wchar.h>
#endif

#include "magic.h"
#include "nan.h"

using namespace node;
using namespace v8;

static Persistent<FunctionTemplate> constructor;
static const char* fallbackPath;

class DetectWorker : public NanAsyncWorker {
  public:
    DetectWorker(NanCallback* callback, const char* d, uint32_t len, const char* path, int flags)
      : NanAsyncWorker(callback),
        data(static_cast<char *>(malloc(len))),
        dataLen(len),
        dataIsPath(false),
        path(path),
        flags(flags),
        free_error(true),
        result(NULL) {
      memcpy(data, d, len);
    }

    DetectWorker(NanCallback* callback, const char* dataPath, const char* path, int flags)
      : NanAsyncWorker(callback),
        data(strdup(dataPath)),
        dataLen(0),
        dataIsPath(true),
        path(path),
        flags(flags),
        free_error(true),
        result(NULL)
        {
    }

    virtual ~DetectWorker () {
      if (result)
        free((void*)result);

      free(data);
    }

    void Execute () {
      const char* r;
      struct magic_set *magic = magic_open(flags | MAGIC_NO_CHECK_COMPRESS);

      if (magic == NULL) {
        UpdateErrmsg(strerror(errno));
      } else if (magic_load(magic, path) == -1
                 && magic_load(magic, fallbackPath) == -1) {
        UpdateErrmsg(magic_error(magic));
        magic_close(magic);
        magic = NULL;
      }

      if (magic == NULL)
        return;

      if (dataIsPath)
        r = magic_file(magic, data);
      else
        r = magic_buffer(magic, (const void*)data, dataLen);

      const char * e = magic_error(magic);
      if (r == NULL || e != NULL)
        UpdateErrmsg(e);
      else
        result = strdup(r);

      magic_close(magic);
    }

  protected:
    void HandleOKCallback () {
      NanScope();

      const unsigned argc = 2;
      Local<Value> argv[argc] = {
        NanNull(),
        NanNew(result)
      };
      callback->Call(argc, argv);
    }

  private:
    void UpdateErrmsg(const char * e) {
      const char * message = e != NULL ? e : "unknown error";
      char * m = static_cast<char *>(::operator new(strlen(message) + 1));
      strcpy(m, message);
      // TODO: Check if commenting out this line breaks anything
      // errmsg = m;
    }

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

    static NAN_METHOD(New) {
      NanScope();
#ifndef _WIN32
      int mflags = MAGIC_SYMLINK;
#else
      int mflags = MAGIC_NONE;
#endif
      char* path = NULL;
      bool use_bundled = true;

      if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use `new` to create instances of this object.");
      }

      if (args.Length() > 1) {
        if (args[1]->IsInt32())
          mflags = args[1]->Int32Value();
        else {
          return NanThrowTypeError("Second argument must be an integer");
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
          return NanThrowTypeError("First argument must be a string or integer");
        }
      }

      Magic* obj = new Magic((use_bundled ? NULL : path), mflags);
      obj->Wrap(args.This());
      obj->Ref();

      NanReturnValue(args.This());
    }

    static NAN_METHOD(DetectFile) {
      NanScope();
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (!args[0]->IsString()) {
        return NanThrowTypeError("First argument must be a string");
      }
      if (!args[1]->IsFunction()) {
        return NanThrowTypeError("Second argument must be a callback function");
      }

      Local<Function> callback = Local<Function>::Cast(args[1]);

      String::Utf8Value str(args[0]->ToString());

      NanAsyncQueueWorker(new DetectWorker(new NanCallback(callback),
          (const char*)*str, obj->mpath, obj->mflags));

      NanReturnUndefined();
    }

    static NAN_METHOD(Detect) {
      NanScope();
      Magic* obj = ObjectWrap::Unwrap<Magic>(args.This());

      if (args.Length() < 2) {
        return NanThrowTypeError("Expecting 2 arguments");
      }
      if (!Buffer::HasInstance(args[0])) {
        return NanThrowTypeError("First argument must be a Buffer");
      }
      if (!args[1]->IsFunction()) {
        return NanThrowTypeError("Second argument must be a callback function");
      }

      Local<Function> callback = Local<Function>::Cast(args[1]);
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 10
      Local<Object> buffer_obj = args[0]->ToObject();
#else
      Local<Value> buffer_obj = args[0];
#endif
      NanAsyncQueueWorker(new DetectWorker(new NanCallback(callback),
          Buffer::Data(buffer_obj), Buffer::Length(buffer_obj),
          obj->mpath, obj->mflags));

      NanReturnUndefined();
    }

    static NAN_METHOD(SetFallback) {
      NanScope();

      if (fallbackPath)
        free((void*)fallbackPath);

      if (args.Length() > 0 && args[0]->IsString()
          && args[0]->ToString()->Length() > 0) {
        String::Utf8Value str(args[0]->ToString());
        fallbackPath = strdup((const char*)(*str));
      } else
        fallbackPath = NULL;

      NanReturnUndefined();
    }

    static void Initialize(Handle<Object> target) {
      NanScope();

      Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
      Local<String> name = NanNew("Magic");

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(name);

      NODE_SET_PROTOTYPE_METHOD(tpl, "detectFile", DetectFile);
      NODE_SET_PROTOTYPE_METHOD(tpl, "detect", Detect);
      target->Set(NanNew<String>("setFallback"),
        NanNew<FunctionTemplate>(SetFallback)->GetFunction());

      target->Set(name, tpl->GetFunction());

      NanAssignPersistent(constructor, tpl);
    }
};

extern "C" {
  void init(Handle<Object> target) {
    NanScope();
    Magic::Initialize(target);
  }

  NODE_MODULE(magic, init);
}
