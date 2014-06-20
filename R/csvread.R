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

# TO GENERATE DOCS:
# # in directory above csvread
# # remove contents of csvread.rox/docs to prevent weird errors 
# library(roxygen)
# roxygenize("csvread", "csvread.rox", copy.package=F, unlink.target=T, use.Rd2=T)
# roxygenize(package.dir="csvread", roxygen.dir="csvread.rox")
# To view output: R CMD Rdconv -t txt csvread/man/cm.read.nz.rd 
# To generate PDF: R CMD Rd2pdf -o csvread.pdf csvread

#-------------------------------------------------------------------------------

# dyn.load("cmrlib.so")

#' Given a list of the column types, parses the CSV file and returns a data frame.
#' The first step is counting the number of lines in the file, which can be avoided
#' if the number of rows is known by providing the \code{nrows} argument. 
#' 
#' If number of columns, which is inferred from the number of provided \code{coltypes}, is greater than
#' the actual number of columns, the extra columns are still created. If the number of columns is
#' less than the actual number of columns in the file, the extra columns in the file are ignored.
#' Commas included in double quotes will be considered part of the field, rather than a separator, but
#' double quotes will NOT be stripped. Runaway double quotes will end at the end of the line.
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
#' @param verbose If \code{TRUE}, the function prints number of lines counted in the file.
#' @param delimiter A single character delimiter, defalut is \code{","}.
#' 
#' @return A data frame containing the data from the CSV file.
#' @examples
#' \dontrun{
#'    frm <- cm.read.csv(file="myfile.csv", 
#'           coltypes=c("integer", "longhex", "double", "string", "long"), 
#'           header=F, nrows=10)
#' }
#' @name cm.read.csv
#' @title Fast CSV reader with a given set of column types.
#' @seealso \code{\link{int64}} \code{\link{utils::read.csv}}
#' @keywords csv comma-separated import text
csvread <- function(file, coltypes, header, colnames = NULL, nrows = NULL, verbose=F, delimiter=",")
{
   if (!is.null(nrows)) nrows <- as.double(nrows)
   return(.Call("readCSV", list(filename=file, coltypes=coltypes, nrows=nrows, header=header, 
                     colnames=colnames, verbose=verbose, delimiter=delimiter), PACKAGE="csvread"))
}

#------------------------------------------------------------------------------

