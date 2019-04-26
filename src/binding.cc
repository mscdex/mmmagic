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

class DetectRequest : public Nan::AsyncResource {
public:
  DetectRequest(Local<Function> callback_, const char* magic_source_,
                size_t source_len_, bool source_is_path_, int flags_)
    : Nan::AsyncResource("mmmagic:DetectRequest"),
      magic_source(magic_source_),
      source_len(source_len_),
      source_is_path(source_is_path_),
      flags(flags_) {
    callback.Reset(callback_);

    request.data = this;
    free_error = true;
    error_message = nullptr;
    result = nullptr;
  }

  ~DetectRequest() {
    callback.Reset();
    data_buffer.Reset();
    if (data_is_path)
      free(data);
    if (free_error)
      free(error_message);
    free((void*)result);
  }

  uv_work_t request;
  Nan::Persistent<Function> callback;

  char* data;
  size_t data_len;
  bool data_is_path;
  Nan::Persistent<Object> data_buffer;

  // libmagic info
  const char* magic_source;
  size_t source_len;
  bool source_is_path;
  int flags;

  bool free_error;
  char* error_message;

  const char* result;
};

static Nan::Persistent<Function> constructor;
static const char* fallbackPath;

class Magic : public ObjectWrap {
public:
    Nan::Persistent<Object> mgc_buffer;
    size_t mgc_buffer_len;
    const char* msource;
    int mflags;

    Magic(const char* path, int flags) {
      if (path != nullptr) {
        /* Windows blows up trying to look up the path '(null)' returned by
           magic_getpath() */
        if (strncmp(path, "(null)", 6) == 0)
          path = nullptr;
      }
      msource = (path == nullptr ? strdup(fallbackPath) : path);

      // When returning multiple matches, MAGIC_RAW needs to be set so that we
      // can more easily parse the output into an array for the end user
      if (flags & MAGIC_CONTINUE)
        flags |= MAGIC_RAW;

      mflags = flags;
    }

    Magic(Local<Object> buffer, int flags) {
      mgc_buffer.Reset(buffer);
      mgc_buffer_len = Buffer::Length(buffer);
      msource = Buffer::Data(buffer);

      // When returning multiple matches, MAGIC_RAW needs to be set so that we
      // can more easily parse the output into an array for the end user
      if (flags & MAGIC_CONTINUE)
        flags |= MAGIC_RAW;

      mflags = flags;
    }

    ~Magic() {
      if (!mgc_buffer.IsEmpty())
        mgc_buffer.Reset();
      else if (msource != nullptr)
        free((void*)msource);
      msource = nullptr;
    }

    static void New(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Nan::HandleScope();
#ifndef _WIN32
      int magic_flags = MAGIC_SYMLINK;
#else
      int magic_flags = MAGIC_NONE;
#endif
      Magic* obj;

      if (!args.IsConstructCall())
        return Nan::ThrowTypeError("Use `new` to create instances of this object.");

      if (args.Length() > 1) {
        if (args[1]->IsInt32())
          magic_flags = Nan::To<int32_t>(args[1]).FromJust();
        else
          return Nan::ThrowTypeError("Second argument must be an integer");
      }

      if (args.Length() > 0) {
        if (args[0]->IsString()) {
          Nan::Utf8String str(args[0]);
          char* path = strdup((const char*)(*str));
          obj = new Magic(path, magic_flags);
        } else if (Buffer::HasInstance(args[0])) {
          obj = new Magic(args[0].As<Object>(), magic_flags);
        } else if (args[0]->IsInt32()) {
          magic_flags = Nan::To<int32_t>(args[0]).FromJust();
          obj = new Magic(nullptr, magic_flags);
        } else if (args[0]->IsBoolean() && !Nan::To<bool>(args[0]).FromJust()) {
          char* path = strdup(magic_getpath(nullptr, 0/*FILE_LOAD*/));
          obj = new Magic(path, magic_flags);
        } else {
          return Nan::ThrowTypeError(
            "First argument must be a string, Buffer, or integer"
          );
        }
      } else {
        obj = new Magic(nullptr, magic_flags);
      }

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

      Nan::Utf8String str(args[0]);

      DetectRequest* detect_req = new DetectRequest(callback,
                                                    obj->msource,
                                                    obj->mgc_buffer_len,
                                                    obj->mgc_buffer.IsEmpty(),
                                                    obj->mflags);
      detect_req->data = strdup((const char*)*str);
      detect_req->data_is_path = true;

      int status = uv_queue_work(uv_default_loop(),
                                 &detect_req->request,
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
      Local<Object> buffer_obj = args[0].As<Object>();

      DetectRequest* detect_req = new DetectRequest(callback,
                                                    obj->msource,
                                                    obj->mgc_buffer_len,
                                                    obj->mgc_buffer.IsEmpty(),
                                                    obj->mflags);
      detect_req->data = Buffer::Data(buffer_obj);
      detect_req->data_len = Buffer::Length(buffer_obj);
      detect_req->data_buffer.Reset(buffer_obj);
      detect_req->data_is_path = false;

      int status = uv_queue_work(uv_default_loop(),
                                 &detect_req->request,
                                 Magic::DetectWork,
                                 (uv_after_work_cb)Magic::DetectAfter);
      assert(status == 0);

      return args.GetReturnValue().Set(args.This());
    }

    static void DetectWork(uv_work_t* req) {
      DetectRequest* detect_req = static_cast<DetectRequest*>(req->data);
      const char* result;
      struct magic_set* magic = magic_open(detect_req->flags
                                           | MAGIC_NO_CHECK_COMPRESS
                                           | MAGIC_ERROR);

      if (magic == nullptr) {
#if NODE_MODULE_VERSION <= 0x000B
        detect_req->error_message =
          strdup(uv_strerror(uv_last_error(uv_default_loop())));
#else
// XXX libuv 1.x currently has no public cross-platform function to convert an
//     OS-specific error number to a libuv error number. `-errno` should work
//     for *nix, but just passing GetLastError() on Windows will not work ...
# ifdef _MSC_VER
        detect_req->error_message = strdup(uv_strerror(GetLastError()));
# else
        detect_req->error_message = strdup(uv_strerror(-errno));
# endif
#endif
      } else if (detect_req->source_is_path) {
        if (magic_load(magic, detect_req->magic_source) == -1
            && magic_load(magic, fallbackPath) == -1) {
          detect_req->error_message = strdup(magic_error(magic));
          magic_close(magic);
          magic = nullptr;
        }
      } else if (magic_load_buffers(magic,
                                    (void**)&detect_req->magic_source,
                                    &detect_req->source_len,
                                    1) == -1) {
        detect_req->error_message = strdup(magic_error(magic));
        magic_close(magic);
        magic = nullptr;
      }

      if (magic == nullptr)
        return;

      if (detect_req->data_is_path) {
#ifdef _WIN32
        // open the file manually to help cope with potential unicode characters
        // in filename
        const char* ofn = detect_req->data;
        int flags = O_RDONLY | O_BINARY;
        int fd = -1;
        int wLen;
        wLen = MultiByteToWideChar(CP_UTF8, 0, ofn, -1, nullptr, 0);
        if (wLen > 0) {
          wchar_t* wfn = (wchar_t*)malloc(wLen * sizeof(wchar_t));
          if (wfn) {
            int wret = MultiByteToWideChar(CP_UTF8, 0, ofn, -1, wfn, wLen);
            if (wret != 0)
              _wsopen_s(&fd, wfn, flags, _SH_DENYNO, _S_IREAD);
            free(wfn);
            wfn = nullptr;
          }
        }
        if (fd == -1) {
          detect_req->free_error = false;
          detect_req->error_message = "Error while opening file";
          magic_close(magic);
          return;
        }
        result = magic_descriptor(magic, fd);
        _close(fd);
#else
        result = magic_file(magic, detect_req->data);
#endif
      } else {
        result = magic_buffer(magic,
                              (const void*)detect_req->data,
                              detect_req->data_len);
      }

      if (result == nullptr) {
        const char* error = magic_error(magic);
        if (error)
          detect_req->error_message = strdup(error);
      } else {
        detect_req->result = strdup(result);
      }

      magic_close(magic);
    }

    static void DetectAfter(uv_work_t* req) {
      Nan::HandleScope scope;
      DetectRequest* detect_req = static_cast<DetectRequest*>(req->data);
      Local<Function> callback = Nan::New(detect_req->callback);
      Local<Object> target = Nan::New<Object>();

      if (detect_req->error_message) {
        Local<Value> err = Nan::Error(detect_req->error_message);
        Local<Value> argv[1] = { err };
        detect_req->runInAsyncScope(target, callback, 1, argv);
      } else {
        Local<Value> argv[2];
        int multi_result_flags =
          (detect_req->flags & (MAGIC_CONTINUE | MAGIC_RAW));

        argv[0] = Nan::Null();

        if (multi_result_flags == (MAGIC_CONTINUE | MAGIC_RAW)) {
          Local<Array> results = Nan::New<Array>();
          if (detect_req->result) {
            uint32_t i = 0;
            const char* result_end =
              detect_req->result + strlen(detect_req->result);
            const char* last_match = detect_req->result;
            const char* cur_match;
            while (true) {
              if (!(cur_match = strstr(last_match, "\n- "))) {
                // Append remainder string
                if (last_match < result_end) {
                  Nan::Set(Local<Object>::Cast(results),
                           i,
                           Nan::New<String>(last_match).ToLocalChecked());
                }
                break;
              }

              size_t match_len = (cur_match - last_match);
              char* match = new char[match_len + 1];
              strncpy(match, last_match, match_len);
              match[match_len] = '\0';

              Nan::Set(Local<Object>::Cast(results),
                       i++,
                       Nan::New<String>(match).ToLocalChecked());

              delete[] match;
              last_match = cur_match + 3;
            }
          }
          argv[1] = Local<Value>(results);
        } else if (detect_req->result) {
          argv[1] =
            Local<Value>(Nan::New<String>(detect_req->result).ToLocalChecked());
        } else  {
          argv[1] = Local<Value>(Nan::New<String>().ToLocalChecked());
        }

        detect_req->runInAsyncScope(target, callback, 2, argv);
      }

      delete detect_req;
    }

    static void SetFallback(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      if (fallbackPath)
        free((void*)fallbackPath);

      fallbackPath = nullptr;
      if (args.Length() > 0 && args[0]->IsString()) {
        Nan::Utf8String str(args[0]);
        if (str.length() > 0)
          fallbackPath = strdup((const char*)(*str));
      }

      return args.GetReturnValue().Set(args.This());
    }

    static void Initialize(Local<Object> target) {

      Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(Nan::New<String>("Magic").ToLocalChecked());
      Nan::SetPrototypeMethod(tpl, "detectFile", DetectFile);
      Nan::SetPrototypeMethod(tpl, "detect", Detect);

      constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
      Nan::Set(target,
               Nan::New<String>("setFallback").ToLocalChecked(),
               Nan::GetFunction(
                 Nan::New<FunctionTemplate>(SetFallback)
               ).ToLocalChecked()).FromJust();

      Nan::Set(target,
               Nan::New<String>("Magic").ToLocalChecked(),
               Nan::GetFunction(tpl).ToLocalChecked()).FromJust();
    }
};

extern "C" {
  void init(Local<Object> target) {
    Nan::HandleScope();
    Magic::Initialize(target);
  }

  NODE_MODULE(magic, init);
}
