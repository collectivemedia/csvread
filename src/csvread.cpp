//-------------------------------------------------------------------------------
//
// Package csvread
//
// R interface for readCSV and associated functions
//
// Sergei Izrailev, 2011-2015
//-------------------------------------------------------------------------------
// Copyright 2011-2014 Collective, Inc.
// Copyright 2015, 2018 Jabiru Ventures LLC
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

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "SfiDelimitedRecordSTD.h"
#include "CMLineStream.h"
#include "CMRDataCollector.h"

#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include <errno.h>

/*

namespace cm
{

//-----------------------------------------------------------------------------

/// Minimum of two values
template<typename T> T cmMin(T a, T b) { return a < b ? a : b; }
/// Maximum of two values
template<typename T> T cmMax(T a, T b) { return a > b ? a : b; }


//-----------------------------------------------------------------------------

} // namespace cm

*/

// df <- csvread("inst/10rows.csv", coltypes=c("longhex", "string", "double", "integer", "long"), header = FALSE, nrows = 10)

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
/// - na.strings - (required) a vector of strings treated as NA; can include an empty string
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

   SEXP rnastrings = getListElement(rschema, "na.strings");
   if (rnastrings == R_NilValue || length(rnastrings) == 0) error("c_readCSV: missing 'na.strings' in the argument list");

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

   // Get the na.strings

   CMRNAStrings nastrings;
   for (int i = 0, n = length(rnastrings); i < n; i++)
   {
      nastrings.add(CHAR(STRING_ELT(rnastrings, i)));
   }


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
   if (hasHeader)
   {
      getline(istr, buffer);
      rec = buffer.c_str();
      for (int i = 0, n = rec.size(); i < n; i++)
      {
         headers.push_back(rec[i]);
      }
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

   for (int i = namecnt, n = headers.size(); i < n && namecnt < ncols; i++)
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
      if (strcmp(CHAR(STRING_ELT(rcoltypes, i)), "integer64") == 0)
      {
         SET_VECTOR_ELT(rframe, i, allocVector(REALSXP, nrows));
         lst[i] = new CMRDataCollectorLong(10);
         lst[i]->attach(VECTOR_ELT(rframe, i));

         SEXP cls;
         PROTECT(cls = allocVector(STRSXP, 1));
         SET_STRING_ELT(cls, 0, mkChar("integer64"));
         classgets(VECTOR_ELT(rframe, i), cls);
         UNPROTECT(1);
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
      rec.split(s, lstr.len());
      for (int i = 0, nr = rec.size(); i < ncols; i++)
      {
         if (i >= nr) lst[i]->append("", nastrings);
         else lst[i]->append(rec.get(i), nastrings);
      }
    }

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
