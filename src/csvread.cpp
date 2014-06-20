#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "SfiDelimitedRecordSTD.h"
#include "CMLineStream.h"

#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include <errno.h>

// assume sizeof(double) >= sizeof(CMInt64)
typedef int_fast64_t CMInt64;

//static const union CMRLongNA { CMInt64 L; double D; } NA_LONG = { -9223372036854775807LL - 1LL };

static const union CMRLongNA { CMInt64 L; double D; } NA_LONG = { -LLONG_MAX - 1 };

char* lltoa(CMInt64 val, char* buf, int base){

    *buf = '\0';

    int i = 62;
    int sign = (val < 0);
    if(sign) val = -val;

    if (val == 0) {
       buf[0] = '0';
       buf[1] = '\0';
       return buf;
    }

    for(; val && i ; --i, val /= base) {
        buf[i] = "0123456789abcdef"[val % base];
    }

    if(sign) {
        buf[i--] = '-';
    }
    return &buf[i+1];

}

namespace cm
{

//-----------------------------------------------------------------------------

/// Minimum of two values
template<typename T> T cmMin(T a, T b) { return a < b ? a : b; }
/// Maximum of two values
template<typename T> T cmMax(T a, T b) { return a > b ? a : b; }

//-----------------------------------------------------------------------------

template <typename T>
class CMVectorWrapper
{
private:
   /// The storage for external data.
   T* m_data;
   /// The number of elements in the vector.
   size_t m_count;
   /// The storage capacity of m_data.
   size_t m_capacity;

public:

   /// Initializes the vector to the given size and takes a non-const pointer to the external storage.
   explicit CMVectorWrapper(int capacity = 0, T* ptr = 0) : m_data(ptr), m_count(0), m_capacity(capacity)
   {
   }

   /// Copy constructor.
   CMVectorWrapper(const CMVectorWrapper<T>& vec)
   {
      *this = vec;
   }

   ~CMVectorWrapper()
   {
   }

   /// Points the vector to the same storage as vec.
   CMVectorWrapper<T>& operator=(const CMVectorWrapper<T>& vec)
   {
      m_data = vec.m_data;
      m_count = vec.m_count;
      m_capacity = vec.m_capacity;
      return *this;
   }

   /// Sets the vector to point to the external storage ptr of the given capacity.
   /// Note that the size of the vector is set to 0. To use all the capacity, also call
   /// \c resize(capacity).
   void attach(int capacity, T* ptr)
   {
      m_count = 0;
      m_capacity = capacity;
      m_data = ptr;
   }

   /// Returns the i-th element of the vector. Can be used as an l-value.
   T& operator[](int i)
   {
      return m_data[i];
   }
   /// Returns the i-th element of the vector.
   const T& operator[](int i) const
   {
      return m_data[i];
   }


   //-------------------------------------------------
   // Size management
   //-------------------------------------------------

   /// Returns the size of the vector.
   int size() const
   {
      return m_count;
   }

   /// Returns the vector's storage capacity. If the vector is attached to external storage,
   /// returns the external storage capacity.
   int capacity() const
   {
      return m_capacity;
   }

   /// Sets the vector size to the smaller of n and m_capacity.
   void resize(size_t n)
   {
      m_count = n > m_capacity ? m_capacity : n;
   }

   /// Sets the number of elements to zero without releasing the memory. Use pack() to also release the memory.
   void clear()
   {
      m_count = 0;
   }

   /// Appends t to the end of the vector up to the vector's capacity. Returns false if capacity is already exhausted.
   bool push_back(const T& t)
   {
      if (m_count >= m_capacity) return false;
      m_data[m_count++] = t;
      return true;
   }

   /// Returns the pointer to the internal storage.
   T* data()
   {
      return m_data;
   }

   /// Returns the pointer to the internal storage.
   const T* data() const
   {
      return m_data;
   }

};

//-----------------------------------------------------------------------------
//
// CMRDataCollector - A base class for parsing and collecting vectors of data in R
//
//-----------------------------------------------------------------------------
/// Base class
class CMRDataCollector
{
protected:
public:
   CMRDataCollector() {}
   virtual ~CMRDataCollector() {}

   /// Parse and append an element to the collection. Returns false if there was a parse error.
   virtual bool append(const char* s) = 0;
   /// Returns the size of the collection.
   virtual int size() const = 0;
   /// Returns the storage capacity of the external storage.
   virtual int capacity() const = 0;
   /// Clears the collection.
   virtual void clear() = 0;
   /// Attaches to storage allocated in rvec.
   virtual void attach(SEXP rvec) = 0;
   /// Sets the size of the vector to the smaller of n and its capacity.
   virtual void resize(int n) = 0;
};

//-----------------------------------------------------------------------------

/// String data collector.
class CMRDataCollectorStr : public CMRDataCollector
{
protected:
   /// A STRSXP vector pre-allocated by the class user.
   SEXP m_data;
   /// Cached capacity of the vector.
   int m_capacity;
   /// Number of inserted elements.
   int m_count;

public:
   CMRDataCollectorStr() {}
   virtual ~CMRDataCollectorStr() {}

   /// Attaches to STRSXP vector. Note that a \b pointer to \c SEXP must be passed.
   virtual void attach(SEXP rvec)
   {
      m_capacity = length(rvec);
      m_count = 0;
      m_data = rvec;
   }

   /// Parse and append an element to the collection. Returns false if there was a parse error.
   virtual bool append(const char* s)
   {
      if (s == 0 || m_count >= m_capacity) return false;
      if (strcmp(s, "NULL") == 0)
         SET_STRING_ELT(m_data, m_count++, NA_STRING);
      else
         SET_STRING_ELT(m_data, m_count++, mkChar(s));
      return true;
   }
   /// Returns the size of the collection.
   virtual int size() const
   {
      return m_count;
   }
   /// Returns the size of the collection.
   virtual int capacity() const
   {
      return m_capacity;
   }
   /// Clears the collection.
   virtual void clear()
   {
      m_count = 0;
   }
   /// Sets the vector size to the smaller of n and m_capacity.
   virtual void resize(int n)
   {
      m_count = n > m_capacity ? m_capacity : n;
   }

   SEXP data() const
   {
      return m_data;
   }
};

//-----------------------------------------------------------------------------

/// Int32 data collector.
class CMRDataCollectorInt : public CMRDataCollector
{
protected:
   CMVectorWrapper<int> m_data;
public:
   CMRDataCollectorInt() {}
   virtual ~CMRDataCollectorInt() {}

   /// Attaches to INTSXP vector. Note that a \b pointer to \c SEXP must be passed.
   virtual void attach(SEXP rvec)
   {
      m_data.attach(length(rvec), INTEGER(rvec));
   }
   /// Parse and append an element to the collection. Returns false if there was a parse error.
   virtual bool append(const char* s)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_INTEGER);
         return false;
      }

      char* p;
      int n = (int) strtol(s, &p, 10);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_INTEGER);
         return false;
      }
      return m_data.push_back(n);
   }
   /// Returns the size of the collection.
   virtual int size() const
   {
      return m_data.size();
   }
   /// Returns the size of the collection.
   virtual int capacity() const
   {
      return m_data.capacity();
   }
   /// Clears the collection.
   virtual void clear()
   {
      m_data.clear();
   }
   /// Sets the vector size to the smaller of n and m_capacity.
   virtual void resize(int n)
   {
      m_data.resize(n);
   }

   /// Returns a pointer to the contiguous data store.
   const int* data() const
   {
      return &m_data[0];
   }
};

//-----------------------------------------------------------------------------

/// double data collector.
class CMRDataCollectorDbl : public CMRDataCollector
{
protected:
   CMVectorWrapper<double> m_data;
public:
   CMRDataCollectorDbl() {}
   virtual ~CMRDataCollectorDbl() {}

   /// Attaches to REALXP vector.
   virtual void attach(SEXP rvec)
   {
      m_data.attach(length(rvec), REAL(rvec));
   }
   /// Parse and append an element to the collection. Returns false if there was a parse error.
   virtual bool append(const char* s)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_REAL);
         return false;
      }

      char* p;
      double x = strtod(s, &p);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_REAL);
         return false;
      }
      return m_data.push_back(x);
   }
   /// Returns the size of the collection.
   virtual int size() const
   {
      return m_data.size();
   }
   /// Returns the size of the collection.
   virtual int capacity() const
   {
      return m_data.capacity();
   }
   /// Clears the collection.
   virtual void clear()
   {
      m_data.clear();
   }
   /// Sets the vector size to the smaller of n and m_capacity.
   virtual void resize(int n)
   {
      m_data.resize(n);
   }

   /// Returns a pointer to the contiguous data store.
   const double* data() const
   {
      return &m_data[0];
   }
};

//-----------------------------------------------------------------------------

/// CMInt64 data collector.
class CMRDataCollectorLong : public CMRDataCollectorDbl
{
protected:
   int m_base;
public:
   /// Base is the base used for conversion of string to CMInt64.
   CMRDataCollectorLong(int base = 10) : m_base(base) {}
   ~CMRDataCollectorLong() {}

   /// Sets the base of the number representation in the strings passed to append().
   void setBase(int base)
   {
      m_base = base;
   }

   /// Returns the base of the number representation in the strings passed to append().
   int getBase() const
   {
      return m_base;
   }

   /// Parse and append an element to the collection. Returns false if there was a parse error.
   virtual bool append(const char* s)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_LONG.D);
         return false;
      }

      char* p;
      CMInt64 u = strtoll(s, &p, m_base);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_LONG.D);
         return false;
      }
//      return m_data.push_back(*((double*) &u));
      double d;
      // assume sizeof(double) >= sizeof(CMInt64)
      memcpy(&d, &u, sizeof(u));
      return m_data.push_back(d);
   }
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

} // namespace cm

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

using namespace cm;

extern "C"
{
//-----------------------------------------------------------------------------

/// Get the list element named str or return NULL - from the manual.
SEXP getListElement(SEXP list, const char* str)
{
   SEXP elmt = R_NilValue;
   SEXP names = getAttrib(list, R_NamesSymbol);
   for (int i = 0, n = length(list); i < n; i++)
   {
      if (strcmp(CHAR(STRING_ELT(names, i)), str) == 0)
      {
         elmt = VECTOR_ELT(list, i);
         break;
      }
   }
   return elmt;
}

//-----------------------------------------------------------------------------

/// A vector sum for testing.
SEXP vecSum(SEXP rvec)
{
   double value = 0;
   double* vec = REAL(rvec);
   int n = length(rvec);
   for (int i = 0; i < n; i++) value += vec[i];
   SEXP res;
   PROTECT(res = allocVector(REALSXP, 1));
   *(REAL(res)) = value;
   UNPROTECT(1);
   return res;
}

//-----------------------------------------------------------------------------

/// A covariance calculation for testing.
SEXP cm_cov(SEXP rvec1, SEXP rvec2)
{
   double sx = 0;
   double sy = 0;
   double sxy = 0;
   double* vec1 = REAL(rvec1);
   double* vec2 = REAL(rvec2);
   int n = length(rvec1);
   if (n != length(rvec2)) error("cm_cov: input vectors are of different length.");
   for (int i = 0; i < n; i++)
   {
      if (ISNAN(vec1[i]) || ISNAN(vec2[i])) continue;
      sx += vec1[i];
      sy += vec2[i];
      sxy += vec1[i] * vec2[i];
   }
   SEXP res;
   PROTECT(res = allocVector(REALSXP, 1));
   if (n == 1) *(REAL(res)) = 0;
   else *(REAL(res)) = (sxy - sx * sy / n) / (n - 1);
   UNPROTECT(1);
   return res;
}

//-----------------------------------------------------------------------------

SEXP numLines(SEXP filename)
{
   const char* fname = CHAR(STRING_ELT(filename, 0));
   CMLineStream lstr(fname);
   char* s;
   int n = 0;
   while ((s = lstr.getline()))
   {
      //Rprintf("--->%s\n", s);
      n++;
   }
   SEXP ret;
   PROTECT(ret = allocVector(INTSXP,1));
   INTEGER(ret)[0] = n;
   UNPROTECT(1);
   return ret;
}

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
         lltoa(xi, s, 10);
         SET_STRING_ELT(res, i, mkChar(s));
      }
   }

   UNPROTECT(1);
   return res;

}

//-----------------------------------------------------------------------------

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
         lltoa(xi, s, 16);
         SET_STRING_ELT(res, i, mkChar(s));
      }
   }

   UNPROTECT(1);
   return res;

}

//-----------------------------------------------------------------------------

SEXP naStringTest()
{
   SEXP res;
   PROTECT(res = allocVector(STRSXP, 1));
   SET_STRING_ELT(res, 0, NA_STRING);
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

// dyn.load("cmrlib.so")
// lst <- .Call("readCSV", list(filename="blah.csv", coltypes=c("integer", "integer", "double", "string"), nrows=10))

/// Reads a CSV file according to provided schema.
/// The argument is a list of the following structure:
/// - filename - name of the CSV file
/// - coltypes - (required) vector of column types; accepted types are integer, logical, string, double
/// - nrows    - resulting number of rows, overriding actual number of rows in the file; saves time
/// - header   - TRUE (default) or FALSE; source of column names if \c colnames is not provided
/// - colnames - column names for all columns; overrides header names when present
/// - verbose  - flag indicating if progress messages should be printed.
/// - delimiter - one-character delimiter (default is comma).
/// If number of columns, which is inferred from the number of provided coltypes, is greater than
/// the actual number of columns, the extra columns are still created. If the number of columns is
/// less than the actual number of columns in the file, the extra columns in the file are ignored.
SEXP readCSV(SEXP rschema)
{
   // Check the arguments.

   if (!isNewList(rschema))
   {
      error("c_readCSV: expecting a list with schema as the only argument");
   }
   SEXP rfilename = getListElement(rschema, "filename");
   if (rfilename == R_NilValue) error("c_readCSV: missing 'filename' in the argument list");
   string filename(CHAR(STRING_ELT(rfilename, 0)));

   SEXP rcoltypes = getListElement(rschema, "coltypes");
   if (rcoltypes == R_NilValue || length(rcoltypes) == 0) error("c_readCSV: missing 'coltypes' in the argument list");

   SEXP rcolnames = getListElement(rschema, "colnames");

   SEXP rheader = getListElement(rschema, "header");
   bool hasHeader = true;
   if (rheader != R_NilValue) hasHeader = *(LOGICAL(rheader));

   SEXP rnrows = getListElement(rschema, "nrows");
   int nrows = 0;
   if (rnrows != R_NilValue)
   {
      nrows = (int) *(REAL(rnrows));
      if (nrows < 1) error("c_readCSV: 'nrows' must be positive");
   }
   bool verbose = false;
   SEXP rverbose = getListElement(rschema, "verbose");
   PROTECT(rverbose = coerceVector(rverbose, INTSXP));
   if (rverbose != R_NilValue) verbose = (bool) *(INTEGER(rverbose));
   UNPROTECT(1);

   char delim = ',';
   SEXP rdelim = getListElement(rschema, "delimiter");
   if (rdelim != R_NilValue)
   {
      string sdelim(CHAR(STRING_ELT(rdelim, 0)));
      if (strlen(sdelim.c_str()) != 1) error("c_readCSV: delimiter must be a single character");
      delim = sdelim.c_str()[0];
   }

   int ncols = length(rcoltypes);

   // Before going any further, check if the file is readable.

   //Rprintf("Trying to load %s\n", filename.c_str());
   ifstream istr(filename.c_str());
   if (istr.fail())
   {
      error("c_readCSV: can't open file %s.", filename.c_str());
   }

   // Read the headers if necessary.

   string buffer;
   SfiDelimitedRecordSTD rec(0, delim);
   vector<string> headers;
   int lineCount = 0;
   if (hasHeader)
   {
      getline(istr, buffer);
      rec = buffer.c_str();
      for (int i = 0, n = rec.size(); i < n; i++)
      {
         headers.push_back(rec[i]);
      }
      lineCount++;
   }
   istr.close();

   // Count the lines if nrows hasn't been provided.

   if (nrows == 0)
   {
      istr.open(filename.c_str());
      const int SZ = 1024 * 1024;
      char buff[SZ];
      int nn = 0;
      int gotsz = 0;
      while (!istr.eof() && !istr.fail())
      {
         istr.read(buff, SZ);
         gotsz = istr.gcount();
//         for (int i = 0; i < gotsz; i++)
//         {
//            if (buff[i] == '\n') nn++;
//         }
         char* p = buff;
         while ((p = (char*) memchr(p, '\n', (buff + gotsz) - p)))
         {
            nn++;
            p++;
         }
      }
      if (gotsz > 0 && buff[gotsz - 1] != '\n') nn++;

      nrows = nn - (int) hasHeader;
      if (verbose) Rprintf("Counted %d lines.\n", nn);
      istr.close();
   }

   // Count the lines if nrows hasn't been provided.

/*   if (nrows == 0)
   {
      if (!hasHeader) getline(istr, buffer);
      while (istr)
      {
         lineCount++;
         getline(istr, buffer);
      }
      //istr.seekg(ios_base::beg); // rewind
      istr.close();
      istr.open(filename.c_str());
      if (hasHeader) getline(istr, buffer);
      nrows = lineCount - (int) hasHeader;
      Rprintf("Counted %d rows\n", nrows);
   }
*/
   // Figure out column names as follows:
   // - the number of names is the number of types
   // - if there are colnames provided, take that
   // - if there are not enough colnames, take the header (if there is one)
   // - if there's still not enough names, fill with "COL<N>" one-based.

   vector<string> colnames;
   int namecnt = 0;
   for (int i = 0, n = length(rcolnames); i < n && namecnt < ncols; i++)
   {
      colnames.push_back(CHAR(STRING_ELT(rcolnames, i)));
      namecnt++;
   }

   for (int i = 0, n = headers.size(); i < n && namecnt < ncols; i++)
   {
      colnames.push_back(headers[i]);
      namecnt++;
   }

   while (namecnt++ < ncols)
   {
      stringstream ss;
      ss << "COL" << namecnt;
      colnames.push_back(ss.str());
   }

   // Allocate and attach collectors to resulting columns.

   vector<CMRDataCollector*> lst(ncols);

   SEXP rframe; // the return value
   PROTECT(rframe = allocVector(VECSXP, ncols));
   if (nrows > 0)
   for (int i = 0; i < ncols; i++)
   {
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "integer") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(INTSXP, nrows));
         lst[i] = new CMRDataCollectorInt();
         lst[i]->attach(VECTOR_ELT(rframe, i));
      }
      else
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "double") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(REALSXP, nrows));
         lst[i] = new CMRDataCollectorDbl();
         lst[i]->attach(VECTOR_ELT(rframe, i));
      }
      else
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "long") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(REALSXP, nrows));
         lst[i] = new CMRDataCollectorLong(10);
         lst[i]->attach(VECTOR_ELT(rframe, i));

         SEXP cls;
         PROTECT(cls = allocVector(STRSXP, 1));
         SET_STRING_ELT(cls, 0, mkChar("int64"));
         classgets(VECTOR_ELT(rframe, i), cls);
         UNPROTECT(1);
      }
      else
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "longhex") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(REALSXP, nrows));
         lst[i] = new CMRDataCollectorLong(16);
         lst[i]->attach(VECTOR_ELT(rframe, i));

         SEXP cls;
         PROTECT(cls = allocVector(STRSXP, 1));
         SET_STRING_ELT(cls, 0, mkChar("int64"));
         classgets(VECTOR_ELT(rframe, i), cls);
         UNPROTECT(1);

         SEXP rb;
         PROTECT(rb = allocVector(INTSXP, 1));
         INTEGER(rb)[0] = 16;
         setAttrib(VECTOR_ELT(rframe, i), install("base"), rb);
         UNPROTECT(1);
      }
      else
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "string") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(STRSXP, nrows));
         lst[i] = new CMRDataCollectorStr();
         lst[i]->attach(VECTOR_ELT(rframe, i));
      }
      else
      {
         UNPROTECT(1);
         for (int k = 0; k < i; k++)
         {
            delete lst[k];
         }
         if (istr.is_open()) istr.close();
         error("c_readCSV: unsupported column type '%s'", CHAR(STRING_ELT(rcoltypes, i)));
      }
   }

   // Load the CSV

   CMLineStream lstr(filename.c_str());
   char* s;
   if (hasHeader) s = lstr.getline();
   while ((s = lstr.getline()))
   {
      //Rprintf("%s\n", s);
/*      rec = s;
      for (int i = 0, n = cm::cmMin(ncols, rec.size()); i < n; i++)
      {
         lst[i]->append(rec[i]);
      }
      */
      rec.split(s, lstr.len());
      for (int i = 0, nr = rec.size(); i < ncols; i++)
      {
         if (i >= nr) lst[i]->append("");
         else lst[i]->append(rec.get(i));
      }
    }
/*

   istr.open(filename.c_str());
   if (hasHeader) getline(istr, buffer);
   getline(istr, buffer);
   rec = buffer.c_str();
   while (istr)
   {
      lineCount++;
      rec = buffer.c_str();

      // TODO: check that record type matches the schema

      for (int i = 0, n = cm::cmMin(ncols, rec.size()); i < n; i++)
      {
         lst[i]->append(rec[i]);
      }
      //cout << buffer << endl;
      //cout << rec.size() << " " << rec[0] << " " << rec[3] << endl;

      getline(istr, buffer);
   }
*/

   // Set the column names

   SEXP rOutColNames;
   PROTECT(rOutColNames = allocVector(STRSXP, ncols));
   for (int i = 0; i < ncols; i++)
   {
      SET_STRING_ELT(rOutColNames, i, mkChar(colnames[i].c_str()));
   }
   setAttrib(rframe, R_NamesSymbol, rOutColNames);
   //for (int i = 0; i < ncols; i++) Rprintf("%d %s\n", i, colnames[i].c_str());


   // Make it a data frame: add class and rownames

   SEXP rOutRowNames;
   PROTECT(rOutRowNames = allocVector(INTSXP, nrows));
   int* iptr = INTEGER(rOutRowNames);
   for (int i = 0; i < nrows; i++)
   {
      iptr[i] = i + 1;
   }
   setAttrib(rframe, R_RowNamesSymbol, rOutRowNames);

   SEXP cls;
   PROTECT(cls = allocVector(STRSXP, 1));
   SET_STRING_ELT(cls, 0, mkChar("data.frame"));
   classgets(rframe, cls);

   // Clean up

   UNPROTECT(4);
   for (int k = 0; k < ncols; k++)
   {
      delete lst[k];
   }
   if (istr.is_open()) istr.close();

   return(rframe);
}

//-----------------------------------------------------------------------------

}
