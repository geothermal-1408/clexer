#include <stdio.h>

#ifndef COMMON_H
#define COMMON_H

#define TODO(fn) fprintf(stderr, "%s:%d: Warning: NOT IMPLEMENTED: %s\n", \
    __FILE__, __LINE__, #fn)

#endif
