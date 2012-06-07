// To avoid GCC pendantic warnings about C++98 incompatibility:
//  - Redefine MOSEK 64bit integral type from 'long long' to 'int64_t'.
//  - Redefine LLONG_MIN and LLONG_MAX with <stdint.h> limits (only available when __STDC_LIMIT_MACROS is defined!!)

#ifndef CPP98_MOSEK_H
#define CPP98_MOSEK_H

#ifndef MSKINT64
  #ifndef __STDC_LIMIT_MACROS
    #define __STDC_LIMIT_MACROS
  #endif

  #include <stdint.h>
  #define MSKINT64 int64_t

  #undef LLONG_MIN
  #define LLONG_MIN INT64_MIN
  #undef LLONG_MAX
  #define LLONG_MAX INT64_MAX
#endif

#ifndef MSKUINT64
  #ifndef __STDC_LIMIT_MACROS
    #define __STDC_LIMIT_MACROS
  #endif

  #include <stdint.h>
  #define MSKUINT64 uint64_t

  #undef ULLONG_MAX
  #define ULLONG_MAX UINT64_MAX
#endif

#include "mosek.h"

#endif
