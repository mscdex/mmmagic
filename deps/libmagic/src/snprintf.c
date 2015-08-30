// XXX: added by mscdex
// MSVS prior to version 2015 do not have a proper snprintf implementation
#include <stdio.h>
#include <stdarg.h>
#if defined(_MSC_VER) && _MSC_VER < 1900
  int vsnprintf(char *outBuf, size_t size, const char *format, va_list ap) {
    int count = -1;

    if (size != 0)
      count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
      count = _vscprintf(format, ap);

    return count;
  }
  int snprintf(char *outBuf, size_t size, const char *format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
  }
#endif
