#-------------------------------------------------------------------------------
#
# Package csvread 
#
# int64 class - not all numeric functionality is implemented.  
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

#' Create, coerce to or test for an int64 vector
int64 <- function(length = 0)
{
   res <- double(length)
   class(res) <- "int64"
   return(res)
}

#-------------------------------------------------------------------------------

is.int64 <- function(x) inherits(x, "int64")

#-------------------------------------------------------------------------------

as.int64 <- function(x, ...) UseMethod("as.int64")

#-------------------------------------------------------------------------------

as.int64.factor <- function(x, ...) as.int64(as.character(x), ...)

#-------------------------------------------------------------------------------

as.int64.character <- function(x, base=10L, ...)
{
   base <- as.integer(base)
   if (base > 36 || base < 2) stop("Can't convert to int64: invalid base.")
	return(.Call("charToInt64", x, base, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

as.int64.numeric <- function(x, ...)
{
   return(.Call("doubleToInt64", as.double(x), PACKAGE="csvread"));
}

#-------------------------------------------------------------------------------

as.int64.NULL <- function(x, ...) 
{
   res <- double()
   class(res) <- "int64"
   return(res)
}

#-------------------------------------------------------------------------------

as.int64.default <- function(x, ...)
{
	if(inherits(x, "int64")) return(x)
	stop(gettextf("do not know how to convert '%s' to class \"int64\"",
					deparse(substitute(x))))
}

#-------------------------------------------------------------------------------

format.int64 <- function(x, na.encode = FALSE, ...)
{
   format(as.character(x), ...)
}

#-------------------------------------------------------------------------------

print.int64 <- function(x, quote = FALSE, ...)
{
	print(format(x), quote = quote, ...)
	invisible(x)
}

#-------------------------------------------------------------------------------

`+.int64` <- function(e1, e2)
{	
	if (nargs() == 1) return(e1)
	if (inherits(e1, "int64") && inherits(e2, "int64"))
   {
      return(.Call("addInt64Int64", e1, e2, PACKAGE="csvread"))
   }
	if (inherits(e1, "int64")) return(.Call("addInt64Int", e1, as.integer(e2), PACKAGE="csvread"))
   if (inherits(e2, "int64")) return(.Call("addInt64Int", e2, as.integer(e1), PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

`-.int64` <- function(e1, e2)
{
   if (nargs() == 1) return(-e1)
   if(inherits(e1, "int64") && inherits(e2, "int64"))
   {
      return(.Call("subInt64Int64", e1, e2, PACKAGE="csvread"))
   }
   if (inherits(e1, "int64")) return(.Call("subInt64Int64", e1, as.int64(e2), PACKAGE="csvread"))
   if (inherits(e2, "int64")) return(.Call("subInt64Int64", as.int64(e1), e2, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

Ops.int64 <- function(e1, e2)
{
	if (nargs() == 1)
		stop("unary ", .Generic, " not defined for int64 objects")
	boolean <- switch(.Generic, "<" =, ">" =, "==" =,
			"!=" =, "<=" =, ">=" = TRUE,
			FALSE)
	if (!boolean) stop(.Generic, " not defined for int64 objects")
	## allow character and numeric args to be coerced to int64
   if (is.character(e1)) e1 <- as.int64(e1)
   else if (is.numeric(e1)) e1 <- as.int64(e1)
   if (is.character(e2)) e2 <- as.int64(e2)
   else if (is.numeric(e2)) e2 <- as.int64(e2)
   NextMethod(.Generic)
}

#-------------------------------------------------------------------------------

Math.int64 <- function (x, ...)
	stop(.Generic, " not defined for int64 objects")

#-------------------------------------------------------------------------------

Summary.int64 <- function (..., na.rm)
{
   stop(.Generic, " not defined for int64 objects")
}

#-------------------------------------------------------------------------------

`[.int64` <- function(x, ..., drop = TRUE)
{
	cl <- oldClass(x)
	class(x) <- NULL
	val <- NextMethod("[")
	class(val) <- cl
	val
}

#-------------------------------------------------------------------------------

`[[.int64` <- function(x, ..., drop = TRUE)
{
	cl <- oldClass(x)
	class(x) <- NULL
	val <- NextMethod("[[")
	class(val) <- cl
	val
}

#-------------------------------------------------------------------------------

`[<-.int64` <- function(x, ..., value)
{
	if(!length(value)) return(x)
	value <- unclass(as.int64(value))
	cl <- oldClass(x)
	class(x) <- NULL
	x <- NextMethod(.Generic)
	class(x) <- cl
	x
}

#-------------------------------------------------------------------------------

#as.character.int64 <- function(x, ...) format(x, ...)
as.character.int64 <- function(x, base=NULL, ...)
{
   if (is.null(base))
   {
      # try attributes first
      i <- match("base", names(attributes(x)))
      if (is.na(i)) base <- 10L
      else base <- as.integer(attr(x, "base"))
   }
   if (base == 10L) return(.Call("int64ToChar", x, PACKAGE="csvread"))
   if (base != 16L) stop(paste("Can't convert int64 to character as base", base))
   return(.Call("int64ToHex", x, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

as.double.int64 <- function(x, ...)
{
   return(.Call("int64ToDouble", x, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

as.integer.int64 <- function(x, ...)
{
   return(.Call("int64ToInteger", x, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

is.na.int64 <- function(x, ...)
{
   return(.Call("isInt64NA", x, PACKAGE="csvread"))
}

#-------------------------------------------------------------------------------

as.data.frame.int64 <- as.data.frame.vector

#-------------------------------------------------------------------------------

as.list.int64 <- function(x, ...)
	lapply(seq_along(x), function(i) x[i])

#-------------------------------------------------------------------------------

c.int64 <- function(..., recursive=FALSE)
	structure(c(unlist(lapply(list(...), unclass))), class="int64")

#-------------------------------------------------------------------------------

mean.int64 <- function (x, ...) stop("mean not implemented for int64")

#-------------------------------------------------------------------------------

sum.int64 <- function(x, ...) stop("sum not implemented for int64")

#-------------------------------------------------------------------------------

seq.int64 <- function(from, to, by, length.out=NULL, along.with=NULL, ...)
{
   stop("seq is not implemented for int64")
}

#-------------------------------------------------------------------------------

rep.int64 <- function(x, ...)
{
	y <- NextMethod()
	structure(y, class="int64")
}

#-------------------------------------------------------------------------------

is.numeric.int64 <- function(x) FALSE

#-------------------------------------------------------------------------------


