//-------------------------------------------------------------------------------
//
// Package csvread
//
// Functions supporting int64 S3 class in R.
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

using namespace std;

#include "int64.h"

#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include <errno.h>

//-----------------------------------------------------------------------------

namespace cm
{

// from http://stackoverflow.com/questions/18858115/c-long-long-to-char-conversion-function-in-embedded-system
static char* cm_lltoa(CMInt64 val, char* buf, int base)
{

    *buf = '\0';

    int i = 62;
    int sign = (val < 0);
    if (sign) val = -val;

    if (val == 0)
    {
       buf[0] = '0';
       buf[1] = '\0';
       return buf;
    }

    for (; val && i ; --i, val /= base)
    {
        buf[i] = "0123456789abcdef"[val % base];
    }

    if (sign)
    {
        buf[i--] = '-';
    }
    return &buf[i+1];
}

}

//-----------------------------------------------------------------------------

using namespace cm;


extern "C"
{
//-----------------------------------------------------------------------------

/// Converts the string representation of long integers in base to CMInt64,
/// stores the results in a double and assigns class int64.
SEXP charToInt64(SEXP rinp, SEXP rbase)
{
   int base = *(INTEGER(rbase));
   int n = length(rinp);
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   char* p;
   double* x = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi = 0;
      SEXP s = STRING_ELT(rinp, i);
      if (s == NA_STRING)
      {
         xi = NA_LONG.L;
      }
      else
      {
         CMInt64 val = strtoll(CHAR(STRING_ELT(rinp, i)), &p, base);
         if (errno == EINVAL || errno == ERANGE)
         {
            xi = NA_LONG.L;
         }
         else
         {
            xi = val;
         }
      }
      // copy int64 to double
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      x[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   if (base == 16)
   {
      SEXP rb;
      PROTECT(rb = allocVector(INTSXP, 1));
      INTEGER(rb)[0] = 16;
      setAttrib(res, install("base"), rb);
      UNPROTECT(1);
   }

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

/// Converts long integers to strings with base 10.
SEXP int64ToChar(SEXP rinp)
{
   int n = length(rinp);
   SEXP res;
   PROTECT(res = allocVector(STRSXP, n));
//   CMInt64* x = (CMInt64*) REAL(rinp);
   char s[100];
   double* x = REAL(rinp);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      // assume sizeof(double) >= sizeof(CMInt64)
      memcpy(&xi, &(x[i]), sizeof(CMInt64));
      if (xi == NA_LONG.L)
      {
         //SET_STRING_ELT(res, i, mkChar("NA"));
         SET_STRING_ELT(res, i, NA_STRING);
      }
      else
      {
//         sprintf(s, "%lld", x[i]);
         cm::cm_lltoa(xi, s, 10);
         SET_STRING_ELT(res, i, mkChar(s));
      }
   }

   UNPROTECT(1);
   return res;

}

//-----------------------------------------------------------------------------

/// Converts long integers to strings with base 16.
SEXP int64ToHex(SEXP rinp)
{
   int n = length(rinp);
   SEXP res;
   PROTECT(res = allocVector(STRSXP, n));
   double* x = REAL(rinp);
   char s[100];
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      memcpy(&xi, &(x[i]), sizeof(CMInt64));

      if (xi == NA_LONG.L)
      {
         SET_STRING_ELT(res, i, NA_STRING);
      }
      else
      {
         if (xi < 0) error("Can't convert a negative number %lld to hex format, item %d.", x[i], i + 1);
         //sprintf(s, "%llx", xi);
         cm::cm_lltoa(xi, s, 16);
         SET_STRING_ELT(res, i, mkChar(s));
      }
   }

   UNPROTECT(1);
   return res;

}

//-----------------------------------------------------------------------------

/// r1 + r2
SEXP addInt64Int64(SEXP r1, SEXP r2)
{
   int n = length(r1);
   if (n != length(r2)) error("Can't add int64 vectors: lengths don't match.");
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   double* x1 = REAL(r1);
   double* x2 = REAL(r2);
   double* x  = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 x1i, x2i, xi;
      memcpy(&x1i, &(x1[i]), sizeof(CMInt64));
      memcpy(&x2i, &(x2[i]), sizeof(CMInt64));

      if (x1i == NA_LONG.L || x2i == NA_LONG.L)
      {
         xi = NA_LONG.L;
      }
      else
      {
         // TODO: check for overflow
         xi = x1i + x2i;
      }
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      x[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

/// r1 + r2, where r1 is int64 and r2 is int
SEXP addInt64Int(SEXP r1, SEXP r2)
{
   int n = length(r1);
   if (n != length(r2)) error("Can't add int64 vectors: lengths don't match.");
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   double* x1 = REAL(r1);
   int* x2 = INTEGER(r2);
   double* x = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 x1i, xi;
      memcpy(&x1i, &(x1[i]), sizeof(CMInt64));

      if (x1i == NA_LONG.L || x2[i] == NA_INTEGER)
      {
         xi = NA_LONG.L;
      }
      else
      {
         // TODO: check for overflow, see http://www.fefe.de/intof.html on multiplication of 64-bit ints
         xi = x1i + (CMInt64) x2[i];
      }
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      x[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

/// Converts integers to int64
SEXP integerToInt64(SEXP r)
{
   int n = length(r);
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   int* xin = INTEGER(r);
   double* xout  = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      if (xin[i] == NA_INTEGER)
      {
         xi = NA_LONG.L;
      }
      else
      {
         xi = (CMInt64) xin[i];
      }
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      xout[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

/// convert doubles to int64
SEXP doubleToInt64(SEXP r)
{
   int n = length(r);
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   double* xin = REAL(r);
   double* xout  = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      if (ISNAN(xin[i]))
      {
         xi = NA_LONG.L;
      }
      else
      {
         xi = (CMInt64) xin[i];
      }
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      xout[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

/// convert int64 to double
SEXP int64ToDouble(SEXP r)
{
   int n = length(r);
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   double* xin = REAL(r);
   double* xd  = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      memcpy(&xi, &(xin[i]), sizeof(CMInt64));
      if (xi == NA_LONG.L)
      {
         xd[i] = NA_REAL;
      }
      else
      {
         xd[i] = (double) xi;
      }
   }

   UNPROTECT(1);
   return res;
}

//-----------------------------------------------------------------------------

/// convert int64 to integer
SEXP int64ToInteger(SEXP r)
{
   int n = length(r);
   SEXP res;
   PROTECT(res = allocVector(INTSXP, n));
   double* xin = REAL(r);
   int* xout  = INTEGER(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      memcpy(&xi, &(xin[i]), sizeof(CMInt64));
      if (xi == NA_LONG.L)
      {
         xout[i] = NA_INTEGER;
      }
      else
      {
         //TODO: check for overflow
         xout[i] = (int) xi;
      }
   }

   UNPROTECT(1);
   return res;
}

//-----------------------------------------------------------------------------

/// Check for NA
SEXP isInt64NA(SEXP r)
{
   int n = length(r);
   SEXP res;
   PROTECT(res = allocVector(LGLSXP, n));
   double* xin = REAL(r);
   int* xout  = LOGICAL(res);
   //memset(xout, 0, sizeof(int) * n);
   for (int i = 0; i < n; i++)
   {
      CMInt64 xi;
      memcpy(&xi, &(xin[i]), sizeof(CMInt64));

      if (xi == NA_LONG.L)
      {
         //Rprintf("%lld TRUE\n", xin[i]);
         xout[i] = 1;
      }
      else
      {
         //Rprintf("%lld FALSE\n", xin[i]);
         xout[i] = 0;
      }
   }

   UNPROTECT(1);
   return res;
}

//-----------------------------------------------------------------------------

/// r1 - r2
SEXP subInt64Int64(SEXP r1, SEXP r2)
{
   int n = length(r1);
   if (n != length(r2)) error("Can't add int64 vectors: lengths don't match.");
   SEXP res;
   PROTECT(res = allocVector(REALSXP, n));
   double* x1 = REAL(r1);
   double* x2 = REAL(r2);
   double* x  = REAL(res);
   for (int i = 0; i < n; i++)
   {
      CMInt64 x1i, x2i, xi;
      memcpy(&x1i, &(x1[i]), sizeof(CMInt64));
      memcpy(&x2i, &(x2[i]), sizeof(CMInt64));

      if (x1i == NA_LONG.L || x2i == NA_LONG.L)
      {
         xi = NA_LONG.L;
      }
      else
      {
         xi = x1[i] - x2[i];
      }
      double d;
      memcpy(&d, &xi, sizeof(CMInt64));
      x[i] = d;
   }

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("int64"));
   classgets(res, cls);

   UNPROTECT(2);
   return res;
}

//-----------------------------------------------------------------------------

}
