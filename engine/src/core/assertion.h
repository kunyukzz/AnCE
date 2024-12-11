#pragma once

#include "define.h"

#define ACASSERT_ENABLED

#ifdef ACASSERT_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugBreak()
#else
#define debugBreak() __builtin_trap()
#endif

ACAPI void report_assert_failure(const char* expression, const char* msg, const char* file, i32 line);

#define ACASSERT(expr)                                                                                                                     \
    {                                                                                                                                      \
        if (expr)                                                                                                                          \
        {                                                                                                                                  \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            report_assert_failure(#expr, "", __FILE__, __LINE__);                                                                          \
            debugBreak();                                                                                                                  \
        }                                                                                                                                  \
    }
#define ACASSERT_MSG(expr, msg)                                                                                                            \
    {                                                                                                                                      \
        if (expr)                                                                                                                          \
        {                                                                                                                                  \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            report_assertion_failure(#expr, msg, __FILE__, __LINE__);                                                                      \
            debugBreak();                                                                                                                  \
        }                                                                                                                                  \
    }

#ifdef _DEBUG
#define ACASSERT_DEBUG(expr)                                                                                                               \
    {                                                                                                                                      \
        if (expr)                                                                                                                          \
        {                                                                                                                                  \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            report_assert_failure(#expr, "", __FILE__, __LINE__);                                                                          \
            debugBreak();                                                                                                                  \
        }                                                                                                                                  \
    }

#else
#define ACASSERT_DEBUG(expr)
#endif

#else
#define ACASSERT(expr)
#define ACASSERT_MSG(expr, msg)
#define ACASSERT_DEBUG(expr)

#endif
