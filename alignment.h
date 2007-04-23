#ifndef ALIGNMENT_H
#define ALIGNMENT_H 1

// XXX: conditionally include this
#include <stdint.h>

#define UNALIGNED_ACCESSES 1 // XXX: need a shake test

// XXX: We use our own typenames here to avoid clashes

#ifdef __intptr_t_defined
typedef uintptr_t oc2e_uintptr_t;
#else
# if !defined(__WORDSIZE) || __WORDSIZE == 64
typedef unsigned long long oc2e_uintptr_t;
# else
typedef unsigned int oc2e_uintptr_t;
# endif
#endif

#if defined(UNALIGNED_ACCESSES) && UNALIGNED_ACCESSES
// eval p here for side-effects
#define aligned(p) ((p) || 1)
#else
#define aligned(p) ((oc2e_uintptr_t)(p) & 1 == 0)
#endif

#endif

