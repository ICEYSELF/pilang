#ifndef UTIL_H
#define UTIL_H

#define NEW(TYPE) (TYPE*)malloc(sizeof(TYPE))
#define NEWN(TYPE, N) (TYPE*)malloc(sizeof(TYPE) * (N))
#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

#endif
