#ifndef CMVectorWrapper_INCLUDED
#define CMVectorWrapper_INCLUDED

namespace cm
{

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

}

#endif // CMVectorWrapper_INCLUDED

