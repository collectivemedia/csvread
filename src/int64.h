//-------------------------------------------------------------------------------
//
// Package csvread
//
// Definitions for the int64 S3 class in R.
//
// Sergei Izrailev, 2011-2014
//-------------------------------------------------------------------------------
// Copyright 2011-2014 Collective, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-------------------------------------------------------------------------------

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
