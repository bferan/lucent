#pragma once

#define LC_ASSERTIONS

#ifdef LC_ASSERTIONS
#include <cassert>

#define LC_ASSERT(expr) assert(expr)
#else
#define LC_ASSERT(...) ((void)0)
#endif

#define LC_ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32= int32_t;
using int64= int64_t;