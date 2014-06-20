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
// SfiVectorLite.h - A light version of std::vector
//
//-----------------------------------------------------------------------------
#ifndef SfiVectorLite_INCLUDED
#define SfiVectorLite_INCLUDED

#include <vector>

//------------------------------------------------------------------------------
//
//   SfiVectorLite
//
//------------------------------------------------------------------------------
/// A light version of std::vector that allows push_back and clear without unnecessary memory operations.
/// The intended purpose is a vector that grows by calling push_back() and then clears by calling clear().
/// In the regular vector, the clear operation releases allocated memory. In this implementation,
/// only the perceived size of the vector is changed, while any memory remains and will be reused on
/// subsequent calls to push_back(). The memory can be released by calling pack().
template <typename T>
class SfiVectorLite
{
private:
   /// The storage for internal data.
   vector<T> m_data;
   /// The number of elements in the vector.
   size_t m_count;

public:

   /// Initializes the vector to the given size.
   explicit SfiVectorLite(int size = 0) : m_count(size)
   {
      m_data.resize(size);
   }

   /// Copy constructor.
   SfiVectorLite(const SfiVectorLite<T>& vec)
   {
      *this = vec;
   }


   ~SfiVectorLite()
   {
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
   /// Returns the size of the vector.
   int length() const
   {
      return m_count;
   }

   /// Returns the vector's storage capacity. If the vector is attached to external storage,
   /// returns the external storage capacity.
   int capacity() const
   {
      return m_data.capacity();
   }

   /// Resizes the underlying vector, but does not change the perceived size.
   void reserve(size_t size)
   {
      if (size > m_data.size())
      {
         m_data.resize(size);
      }
   }
   /// Resizes the vector storage and adds/removes the elements appropriately.
   void resize(size_t size)
   {
      m_data.resize(size);
      m_count = size;
   }


   /// Resizes the vector to the current number of elements. Can be used to free unused space
   /// after removing elements or resizing the vector down.
   void pack()
   {
      m_data.resize(m_count);
   }

   /// Sets the number of elements to zero without releasing the memory. Use pack() to also release the memory.
   void clear()
   {
      m_count = 0;
   }

   /// Appends t to the end of the vector.
   void push_back(const T& t)
   {
      if (m_count + 1 > m_data.size())
      {
         m_data.push_back(t);
         m_count++;
      }
      else
      {
         m_data[m_count++] = t;
      }
   }

   /// Copies the contents of vec into the vector.
   SfiVectorLite<T>& operator=(const SfiVectorLite<T>& vec)
   {
      m_data = vec.m_data;
      m_count = vec.m_count;
      return *this;
   }


};

#endif

