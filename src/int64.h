#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

namespace cm
{
// assume sizeof(double) >= sizeof(CMInt64)
typedef int_fast64_t CMInt64;

//-----------------------------------------------------------------------------

//static const union CMRLongNA { CMInt64 L; double D; } NA_LONG = { -9223372036854775807LL - 1LL };

static const union CMRLongNA { CMInt64 L; double D; } NA_LONG = { LLONG_MIN };

}
