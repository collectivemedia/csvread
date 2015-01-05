//-------------------------------------------------------------------------------
//
// Package csvread
//
// class CMRDataCollector and its derived classes
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

#ifndef CMRDataCollector_INCLUDED
#define CMRDataCollector_INCLUDED

using namespace std;

#include "CMVectorWrapper.h"
#include "int64.h"

#include <R.h>
#include <Rinternals.h>
#include <errno.h>

namespace cm
{

class CMRNAStrings
{
protected:
   /// Vector holding the strings to compare to.
   vector<string> m_nastrings;
   /// Vector holding the lengths of the strings in m_nastrings.
   vector<int> m_lengths;
   /// Flag indicating if empty string is treated as NA.
   bool m_emptyIsNA;

public:

   CMRNAStrings() : m_emptyIsNA(false) {}
   virtual ~CMRNAStrings() {}

   /// Returns TRUE if s is considered an NA.
   bool isNA(const char* s) const
   {
      if (s == 0) return true;
      int len = strlen(s);
      if (len == 0 && m_emptyIsNA) return true;
      for (int i = 0, n = m_nastrings.size(); i < n; i++)
      {
         if (len == m_lengths[i] && m_nastrings[i] == s) return true;
      }
      return false;
   }

   /// Adds a string that should be considered an NA.
   void add(const char* s)
   {
      if (s == 0) return;  // ignore null pointers (just a precaution).
      int len = strlen(s);
      m_nastrings.push_back(s);
      m_lengths.push_back(len);
      if (len == 0) m_emptyIsNA = true;
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
   virtual bool append(const char* s, const CMRNAStrings& nastrings) = 0;
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

   /// Parse and append an element to the collection. Returns false if there was an NA.
   virtual bool append(const char* s, const CMRNAStrings& nastrings)
   {
      if (m_count >= m_capacity) return false;
      if (nastrings.isNA(s))
      {
         SET_STRING_ELT(m_data, m_count++, NA_STRING);
         return false;
      }
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
   virtual bool append(const char* s, const CMRNAStrings& nastrings)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_INTEGER);
         return false;
      }
      if (nastrings.isNA(s))
      {
         m_data.push_back(NA_INTEGER);
         return false;
      }

      char* p;
      int n = (int) strtol(s, &p, 10);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_INTEGER);
         errno = 0;
         return false;
      }
      errno = 0;
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
   virtual bool append(const char* s, const CMRNAStrings& nastrings)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_REAL);
         return false;
      }
      if (nastrings.isNA(s))
      {
         m_data.push_back(NA_REAL);
         return false;
      }

      char* p;
      double x = strtod(s, &p);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_REAL);
         errno = 0;
         return false;
      }
      errno = 0;
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
   virtual bool append(const char* s, const CMRNAStrings& nastrings)
   {
      if (s == 0 || *s == '\0')
      {
         m_data.push_back(NA_LONG.D);
         return false;
      }
      if (nastrings.isNA(s))
      {
         m_data.push_back(NA_LONG.D);
         return false;
      }

      char* p;
      CMInt64 u = strtoll(s, &p, m_base);
      if (errno == EINVAL || errno == ERANGE)
      {
         m_data.push_back(NA_LONG.D);
         errno = 0;
         return false;
      }
//      return m_data.push_back(*((double*) &u));
      // assume sizeof(double) >= sizeof(CMInt64)
      double d;
      memcpy(&d, &u, sizeof(u));
      errno = 0;
      return m_data.push_back(d);
   }
};

//-----------------------------------------------------------------------------

}

#endif // CMRDataCollector_INCLUDED
