// XXX: change by mscdex
// from mingw-w64-crt project

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

int vasprintf(char ** ret, const char * format, va_list ap) {
  int len;
  /* Get Length */
  len = _vsnprintf(NULL, 0, format, ap);
  if (len < 0)
    return -1;
  /* +1 for \0 terminator. */
  *ret = malloc(len + 1);
  /* Check malloc fail*/
  if (!*ret) {
    errno = ENOMEM;
    return -1;
  }
  /* Write String */
  _vsnprintf(*ret, len + 1, format, ap);
  /* Terminate explicitly */
  (*ret)[len] = '\0';
  return len;
}
