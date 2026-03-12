#ifndef COMMON_H
#define COMMON_H

#ifdef __clang__
  #define _COL_ __builtin_COLUMN()
#else
  #define _COL_ 0
#endif

#define TODO(fn) fprintf(stderr, "%s:%d:%d: warning: NOT IMPLEMENTED: %s\n", \
    __FILE__, __LINE__, _COL_, #fn)

#endif
