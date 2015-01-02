#-------------------------------------------------------------------------------
#
# Package csvread 
#
# Function csvread 
# 
# Sergei Izrailev, 2011-2014
#-------------------------------------------------------------------------------
# Copyright 2011-2014 Collective, Inc.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------

#' Given a list of the column types, function \code{csvread} parses the CSV file 
#' and returns a data frame.  
#' 
#' \code{csvread} provides functionality for loading large (10M+ lines) CSV
#' and other delimited files, similar to read.csv, but typically faster and
#' using less memory than the standard R loader. While not entirely general,
#' it covers many common use cases when the types of columns in the CSV file
#' are known in advance. In addition, the package provides a class 'int64',
#' which represents 64-bit integers exactly when reading from a file. The
#' latter is useful when working with 64-bit integer identifiers exported from
#' databases. The CSV file loader supports common column types including 
#' \code{integer}, \code{double}, \code{string}, and \code{int64}, leaving 
#' further type transformations  to the user. 
#' 
#' If number of columns, which is inferred from the number of provided \code{coltypes}, is greater than
#' the actual number of columns, the extra columns are still created. If the number of columns is
#' less than the actual number of columns in the file, the extra columns in the file are ignored.
#' Commas included in double quotes will be considered part of the field, rather than a separator, but
#' double quotes will NOT be stripped. Runaway double quotes will end at the end of the line.
#'
#' See also \code{\link{int64}} for information about dealing with 64-bit 
#' integers when loading data from CSV files. 
#' 
#' @param file Path to the CSV file.
#' @param coltypes A vector of column types, e.g., \code{c("integer", "string")}. 
#'        The accepted types are "integer", "double", "string", "long" and "longhex".
#' \itemize{
#' \item \code{integer} - the column is parsed into an R integer type (32 bit)
#' \item \code{double} - the column is parsed into an R double type
#' \item \code{string} - the column is loaded as character type
#' \item \code{long} - the column is interpreted as the decimal representation of a 64-bit
#'              integer, stored as a double and assigned the \code{\link{int64}} class.
#' \item \code{longhex} - the column is interpreted as the hex representation of a 64-bit
#'              integer, stored as a double and assigned the \code{\link{int64}} class 
#'              with an additional attribute \code{base = 16L} that is used for printing.             
#' \item \code{integer64} - same as \code{long} but produces a column of class \code{integer64},
#'          which should be compatible with package \code{bit64} (untested).
#' \item \code{verbose} - if \code{TRUE}, the function prints number of lines counted in the file.
#' \item \code{delimiter} - a single character delimiter, defalut is \code{","}.
#' } 
#' @param header TRUE (default) or FALSE; indicates whether the file has a header 
#'        and serves as the source of column names if \code{colnames} is not provided.
#' @param colnames Optional column names for the resulting data frame. Overrides the header, if header is present.
#'        If NULL, then the column names are taken from the header, or, if there is no header, 
#'        the column names are set to 'COL1', 'COL2', etc.
#' @param nrows If NULL, the function first counts the lines in the file. This step can be avoided if the number 
#'        of lines is known by providing a value to \code{nrows}. On the other hand, \code{nrows} can be 
#'        used to read only the first lines of the CSV file.
#' @param verbose If \code{TRUE} and \code{nrows} is \code{NULL}, the function prints 
#'        number of lines counted in the file.
#' @param delimiter A single character delimiter, defalut is \code{","}.
#' 
#' @return A data frame containing the data from the CSV file.
#' @examples
#' \dontrun{
#' frm <- csvread("inst/10rows.csv", 
#'    coltypes = c("longhex", "string", "double", "integer", "long"), 
#'    header = FALSE, nrows = 10)
#' frm
#' #               COL1       COL2     COL3 COL4 COL5
#' # 1  11fb89c1558c792 2011-05-06 0.150001 4970 4977
#' # 2  11fb89c1558c792 2011-05-06 0.150001 4970 4987
#' # 3  11fb89c1558c792 2011-05-06 0.150001 5200 5528
#' # 4  11fb89c1558c792 2011-05-06 0.150001 4970 5004
#' # 5  11fb89c1558c792 2011-05-06 0.150001 4970 4980
#' # 6  11fb89c1558c792 2011-05-06 0.150001 4970 5020
#' # 7  11fb89c1558c792 2011-05-06 0.150001 4970 5048
#' # 8  11fb89c1558c792 2011-05-06 0.150001 4970 5035
#' # 9  11fb89c1558c792 2011-05-06 0.150001 4970 4971
#' # 10 11fb89c1558c792 2011-05-06 0.150001 4970 4973
#' 
#' typeof(frm$COL1)
#' # [1] "double"
#' class(frm$COL1)
#' # [1] "int64"
#' 
#' typeof(frm$COL5)
#' # [1] "double"
#' class(frm$COL5)
#' # [1] "int64"
#' }
#' @name csvread
#' @title Fast CSV reader with a given set of column types.
#' @seealso \code{\link{int64}} 
#' @keywords csv comma-separated import text
csvread <- function(file, coltypes, header, colnames = NULL, nrows = NULL, 
      verbose = FALSE, delimiter = ",", na.strings = c("NA", "na", "NULL", "null", ""))
{
   if (!is.null(nrows)) nrows <- as.double(nrows)
   return(.Call("readCSV", list(filename=file, coltypes=coltypes, nrows=nrows, header=header, 
                     colnames=colnames, verbose=verbose, delimiter=delimiter, 
                     na.strings=na.strings), PACKAGE="csvread"))
}

#------------------------------------------------------------------------------

#' \code{map.coltypes} guesses the column types in the CSV file by reading the first
#' \code{nrows} lines. The result can be passed to \code{csvread} as the 
#' \code{coltypes} argument.
#' 
#' @rdname csvread
#' @examples
#' \dontrun{
#' coltypes <- map.coltypes("inst/10rows.csv", header = FALSE)
#' coltypes
#' #       V1        V2        V3        V4        V5 
#' # "string"  "string"  "double" "integer" "integer"  
#' 
#' frm <- csvread(file = "inst/10rows.csv", coltypes = coltypes, header = F, verbose = T)
#' # Counted 10 lines.
#' 
#' frm
#' #               COL1       COL2     COL3 COL4 COL5
#' # 1  11fb89c1558c792 2011-05-06 0.150001 4970 4977
#' # 2  11fb89c1558c792 2011-05-06 0.150001 4970 4987
#' # 3  11fb89c1558c792 2011-05-06 0.150001 5200 5528
#' # 4  11fb89c1558c792 2011-05-06 0.150001 4970 5004
#' # 5  11fb89c1558c792 2011-05-06 0.150001 4970 4980
#' # 6  11fb89c1558c792 2011-05-06 0.150001 4970 5020
#' # 7  11fb89c1558c792 2011-05-06 0.150001 4970 5048
#' # 8  11fb89c1558c792 2011-05-06 0.150001 4970 5035
#' # 9  11fb89c1558c792 2011-05-06 0.150001 4970 4971
#' # 10 11fb89c1558c792 2011-05-06 0.150001 4970 4973
#' typeof(frm$COL1)
#' # [1] "character"
#' class(frm$COL1)
#' # [1] "character"
#' 
#' typeof(frm$COL5)
#' # [1] "integer"
#' class(frm$COL5)
#' # [1] "integer"
#' 
#' frm$COL1 <- as.int64(frm$COL1, base = 16)
#' frm$COL1
#' # [1] "11fb89c1558c792" "11fb89c1558c792" "11fb89c1558c792" "11fb89c1558c792"
#' # [5] "11fb89c1558c792" "11fb89c1558c792" "11fb89c1558c792" "11fb89c1558c792"
#' # [9] "11fb89c1558c792" "11fb89c1558c792"
#' typeof(frm$COL1)
#' # [1] "double"
#' class(frm$COL1)
#' # [1] "int64"
#' 
#' as.character.int64(frm$COL1[1], base = 10)
#' # [1] "80986298828507026"
#' }
map.coltypes <- function(file, header, nrows = 100, delimiter = ",")
{
   df <- read.csv(file, stringsAsFactors  = FALSE, header = header, sep = delimiter, nrows = nrows)
   coltypes <- unlist(lapply(df, function(x) typeof(x)))
   coltypes[coltypes == "logical"] <- "integer"
   coltypes[coltypes == "character"] <- "string"
   return(coltypes)
}

#------------------------------------------------------------------------------
