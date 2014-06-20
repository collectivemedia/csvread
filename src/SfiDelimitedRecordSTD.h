// Copyright (c) 2007-2011 Jabiru Ventures LLC
// Licensing questions should be addressed to jvlicense@jabiruventures.com
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------
//
// SfiDelimitedRecordSTD.h - A record consisting of string fields separated by a delimiter.
//
//-----------------------------------------------------------------------------
#ifndef SfiDelimitedRecordSTD_INCLUDED
#define SfiDelimitedRecordSTD_INCLUDED

#include "SfiVectorLite.h"
#include <string>


//------------------------------------------------------------------------------
//
//   SfiDelimitedRecordSTD
//
//------------------------------------------------------------------------------
/// A record consisting of string fields separated by a delimiter. The functionality
/// is similar to the split() function in Perl or the way awk treats lines.

/// There are two ways of using this class. The first makes a copy of the string.
/// In this case, the class should be initialized with a string and the
/// resulting fields should be accessed by the \c operator[]. The second way is to
/// pass the line to the class as a writable buffer in the \c split function and
/// access the resulting fields via the \c get() method. In this case, the split
/// occurs in-place, i.e., without a copy or memory allocation, but the delimiters
/// in the buffer are overwritten with null characters. The \c get() method
/// should not be called after the buffer was subsequently changed by the caller.
class SfiDelimitedRecordSTD
{
protected:
   /// Buffer with a modified string for fast retrieval.
   string m_buffer;
   char m_delimiter;
   SfiVectorLite<int> m_offsets;
   SfiVectorLite<int> m_lengths;
   char* m_sptr;
   const char m_nullChar;

public:
   explicit SfiDelimitedRecordSTD(const char* str = 0, char delimiter = ',') : m_delimiter(delimiter), m_sptr(0), m_nullChar(0)
   {
      m_offsets.reserve(6);
      m_lengths.reserve(6);
      *this = str;
   }
   SfiDelimitedRecordSTD(const SfiDelimitedRecordSTD& rec) : m_nullChar(0)
   {
      *this = rec;
   }
   ~SfiDelimitedRecordSTD() {}

   /// Copies all data from rec.
   SfiDelimitedRecordSTD& operator=(const SfiDelimitedRecordSTD& rec)
   {
      m_buffer = rec.m_buffer;
      m_delimiter = rec.m_delimiter;
      m_offsets = rec.m_offsets;
      m_lengths = rec.m_lengths;
      m_sptr = rec.m_sptr;
      return *this;
   }

   /// Sets the record to a new string (makes a copy).
   /// Access to the resulting split string is via the \c operator[].
   SfiDelimitedRecordSTD& operator=(const char* str)
   {
      if (str)
      {
         m_buffer = str;
         split();
      }
      else
      {
         clear();
      }
      return *this;
   }

   /// Returns the number of fields in the record.
   int size() const
   {
      return m_offsets.size();
   }

   /// Returns a pointer to the i-th field or an empty string if there are fewer than i fields.
   const char* operator[](int i) const
   {
      int size = m_offsets.size();
      if (size == 0 || i < 0 || i >= size)
      {
         return &m_nullChar; //m_buffer.c_str() + m_offsets[size - 1] + m_lengths[size - 1];
      }
      return m_buffer.c_str() + m_offsets[i];
   }

   /// Returns the length of the n-th field (zero-based) or -1 if there is no such field.
   int length(int n) const
   {
      return n < m_offsets.size() ? m_lengths[n] : -1;
   }

   /// Sets the delimiter character and re-splits the string.
   void setDelimiter(char delim)
   {
      m_delimiter = delim;
   }

   /// Returns the offset of the n-th field (zero-based) in the original string or -1 if there is no such field.
   int offset(int n) const
   {
      return n < m_offsets.size() ? m_offsets[n] : -1;
   }

   /// Splits the \c buf in-place, overwriting delimiters with null characters.
   /// Returns the number of fields in the \c buf. Delimiters inside double quotes are ignored.
   /// \c n is the size of string in \c buf, excluding the terminating null.
   /// Access to fields is provided by \c get(int).
   int split(char* buf, int n)
   {
      if (!buf)
      {
         clear();
         return 0;
      }
      // The code here is identical with that in split() except here we
      // operate on a char* buffer, and split() operates on a std::string.
      m_sptr = buf;
      int start = 0;
      int i;
      m_offsets.clear();
      m_lengths.clear();
      bool insideQuotes = false;
      for (i = 0; i < n; i++)
      {
         if (buf[i] == '"')
         {
            insideQuotes = !insideQuotes;
         }
         if (!insideQuotes && buf[i] == m_delimiter)
         {
            buf[i] = '\0';
            m_offsets.push_back(start);
            m_lengths.push_back(i - start);
            start = i + 1;
         }
      }
      m_offsets.push_back(start);
      m_lengths.push_back(i - start);
      return i ? m_offsets.size() : 0;
   }

   /// Returns a pointer to the i-th field of a split string - for use with split(char*, int) only!!!
   /// If the index i is outside the range of valid fields, a pointer to an empty string is returned.
   const char* get(int i) const
   {
      int size = m_offsets.size();
      if (size == 0 || i < 0 || i >= size)
      {
         return &m_nullChar; //m_sptr + m_offsets[size - 1] + m_lengths[size - 1];
      }
      return m_sptr + m_offsets[i];
   }

protected:

   /// Returns the number of fields in the record. Delimiters inside double quotes are ignored.
   int split()
   {
      int start = 0;
      int i, n;
      m_offsets.clear();
      m_lengths.clear();
      bool insideQuotes = false;
      for (i = 0, n = m_buffer.length(); i < n; i++)
      {
         if (m_buffer[i] == '"')
         {
            insideQuotes = !insideQuotes;
         }
         if (!insideQuotes && m_buffer[i] == m_delimiter)
         {
            m_buffer[i] = '\0';
            m_offsets.push_back(start);
            m_lengths.push_back(i - start);
            start = i + 1;
         }
      }
      m_offsets.push_back(start);
      m_lengths.push_back(i - start);
      return i ? m_offsets.size() : 0;
   }

   /// Clears the record.
   void clear()
   {
      m_buffer.clear();
      m_offsets.clear();
      m_lengths.clear();
      m_sptr = 0;
   }
};

#endif
