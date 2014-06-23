//-------------------------------------------------------------------------------
//
// Package csvread
//
// class CMLineStream
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

#ifndef CMLineStream_INCLUDED
#define CMLineStream_INCLUDED

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

namespace cm
{

//-----------------------------------------------------------------------------
//
// CMLineStream - Utility class to quickly consume lines from text files.
//
//-----------------------------------------------------------------------------
/// A buffered implementation of line reader. The file stream is only read once in big chunks
/// and the returned lines are pointers to internal class storage that will be modified on the
/// next call to \c getline(). The returned lines are intended to be further processed in-place,
/// hence the returned pointer is non-const.
///
/// When end of input is reached, the file is automatically closed.
///
/// Usage:
/// \code
/// // To count lines:
/// CMLineStream lstr(filename);
/// char* s;
/// int nlines = 0;
/// int nchars = 0;
/// while (s = lstr.getline())
/// {
///    nlines++;
///    nchars += lstr.len();
/// }
/// \endcode
///
class CMLineStream
{
   static const int s_bufsz = 1024 * 1024;
protected:
   string m_filename;         ///< Name of the attached file.
   ifstream m_istr;           ///< Input stream.
   char m_buffer[s_bufsz];    ///< Buffer for reading.
   string m_line;             ///< Space to accumulate lines across buffer reads.

   int m_start;               ///< Beginning of the next line in the buffer.
   int m_gcount;              ///< Number of bytes actually read.
   bool m_done;               ///< Flag indicating that getline() quits the next time it's called.
   bool m_bufferEmpty;        ///< Flag indicating that the buffer is empty or exhausted, so another read is needed.
   bool m_linePending;        ///< Flag indicating that there is a line pending from previous buffer.
   int m_len;                 ///< Length of the most recently returned line.

   /// Clears everything.
   void clear()
   {
      m_line.clear();
      m_filename.clear();
      m_gcount = 0;
      m_start = 0;
      m_done = false;
      m_bufferEmpty = true;
      m_linePending = false;
      m_len = 0;
   }
public:
   /// Creates the object and attaches is to the file if provided.
   CMLineStream(const char* filename = 0) 
   {
      clear();
      if (filename) 
      {
         m_filename = filename;
         m_istr.open(filename);
      }
   }
   virtual ~CMLineStream() {}

   /// Opens a file and returns FALSE if failed.
   bool open(const char* filename)
   {
      if (m_istr.is_open()) m_istr.close();
      clear();
      m_istr.open(filename);
      m_filename = filename;
      return !m_istr.fail();
   }
   /// Closes the open file.
   void close()
   {
      if (m_istr.is_open()) m_istr.close();
      clear();
   }

   /// Returns the length of the most recently returned line.
   int len() const
   {
      return m_len;
   }

   /// Returns a non-const pointer to the next line or NULL if end of input.
   char* getline()
   {
      if (m_done)
      {
         close();
         return 0;
      }
      if (m_bufferEmpty)
      {
         // beginning of the file or have read previous buffer
         m_istr.read(m_buffer, s_bufsz);
         m_gcount = m_istr.gcount();
         if (m_gcount == 0)
         {
            // nothing was read
            if (m_linePending)
            {
               m_done = true;
               m_linePending = false;
               m_len = m_line.size();
               return (char*) m_line.c_str();
            }
            m_len = 0;
            return 0;
         }
         m_start = 0;
         m_bufferEmpty = false;

         if (m_gcount < s_bufsz)
         {
            // incomplete buffer,
         }
      }

      // Find the next newline
      int k;
      char* sret = m_buffer + m_start;
      for (k = m_start; k < m_gcount; k++)
      {
         if (m_buffer[k] == '\n')
         {
            // found a newline character
            m_buffer[k] = '\0';

            if (m_linePending)
            {
               // append the current string to the pending line
               m_linePending = false;
               m_line += m_buffer + m_start;
               sret = (char*) m_line.c_str();
               m_len = m_line.size();
            }
            else
            {
               m_len = k - m_start;
            }
            if (k == m_gcount - 1)
            {
               // the newline is the last char of the buffer
               if (m_gcount < s_bufsz)
               {
                  // the current buffer is incomplete, so there's nothing more to read
                  m_done = true;
               }
               else
               {
                  // set up the next buffer read
                  m_bufferEmpty = true;
               }
            }
            else
            {
               m_start = k + 1;
            }
            return sret;
         }
      }

      // No newline found to the end of the buffer.

      if (m_gcount < s_bufsz)
      {
         // the current buffer is incomplete, nothing more to read
         m_done = true;

         // null-terminate the string
         m_buffer[m_gcount] = '\0';

         if (m_linePending)
         {
            // append to the existing line and return
            m_line += sret;
            m_len = m_line.size();
            return (char*) m_line.c_str();
         }
         else
         {
            m_len = k - m_start;
         }
         return sret;
      }

      // Full buffer not ending with an newline, there may be more to read

      m_bufferEmpty = true;

      // have to copy to null-terminate the string
      string s(sret, m_gcount - m_start);

      if (m_linePending)
      {
         // append to the existing line
         m_line += s.c_str();
      }
      else
      {
         // start a new line
         m_line = s;
         m_linePending = true;
      }
      m_len = m_line.size();
      return getline();

   }

};

} // namespace cm

#endif

