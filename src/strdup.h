#ifndef _SEB_STRDUP_H
#define _SEB_STRDUP_H

char *strdup(const char *s);
char *strndup(const char *s, size_t n);

/* Duplicate S, returning an identical alloca'd string.  */
# define strdupa(s)                               \
  (__extension__                                  \
    ({                                        \
      __const char *__old = (s);                          \
      size_t __len = strlen (__old) + 1;                      \
      char *__new = (char *) __builtin_alloca (__len);                \
      (char *) memcpy (__new, __old, __len);                      \
    }))

/* Return an alloca'd copy of at most N bytes of string.  */
# define strndupa(s, n)                               \
  (__extension__                                  \
    ({                                        \
      __const char *__old = (s);                          \
      size_t __len = strnlen (__old, (n));                    \
      char *__new = (char *) __builtin_alloca (__len + 1);            \
      __new[__len] = '\0';                            \
      (char *) memcpy (__new, __old, __len);                      \
    }))

#endif
