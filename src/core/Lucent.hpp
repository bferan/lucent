#pragma once

#define LC_ASSERTIONS

#ifdef LC_ASSERTIONS
#include <cassert>

#define LC_ASSERT(expr) assert(expr)
#else
#define LC_ASSERT(...) ((void)0)
#endif