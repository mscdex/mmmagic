// XXX: change by mscdex
// from:
// http://www.redantigua.com/c-ex-beginner-concat-strings-asprintf.html

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef VA_COPY
# ifdef HAVE_VA_COPY
#  define VA_COPY(dest, src) va_copy(dest, src)
# else
#  ifdef HAVE___VA_COPY
#   define VA_COPY(dest, src) __va_copy(dest, src)
#  else
#   define VA_COPY(dest, src) (dest) = (src)
#  endif
# endif
#endif

#define INIT_SZ 128

int
vasprintf(char **str, const char *fmt, va_list ap)
{
        int ret = -1;
        va_list ap2;
        char *string, *newstr;
        size_t len;

        VA_COPY(ap2, ap);
        if ((string = malloc(INIT_SZ)) == NULL)
                goto fail;

        ret = vsnprintf(string, INIT_SZ, fmt, ap2);
        if (ret >= 0 && ret < INIT_SZ) { /* succeeded with initial alloc */
                *str = string;
        } else if (ret == INT_MAX || ret < 0) { /* Bad length */
                goto fail;
        } else {        /* bigger than initial, realloc allowing for nul */
                len = (size_t)ret + 1;
                if ((newstr = realloc(string, len)) == NULL) {
                        free(string);
                        goto fail;
                } else {
                        va_end(ap2);
                        VA_COPY(ap2, ap);
                        ret = vsnprintf(newstr, len, fmt, ap2);
                        if (ret >= 0 && (size_t)ret < len) {
                                *str = newstr;
                        } else { /* failed with realloc'ed string, give up */
                                free(newstr);
                                goto fail;
                        }
                }
        }
        va_end(ap2);
        return (ret);

fail:
        *str = NULL;
        errno = ENOMEM;
        va_end(ap2);
        return (-1);
}