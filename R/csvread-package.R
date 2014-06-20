#-------------------------------------------------------------------------------
#
# Package csvread 
#
# General description 
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
#' Various R utilities, some having to do with loading CSV files, Netezza access, timing.
#' \tabular{ll}{
#' Package: \tab csvread\cr
#' Type: \tab Package\cr
#' Version: \tab 3.5\cr
#' Date: \tab 2014-05-31\cr
#' License: \tab Apache License, Version 2.0\cr
#' Author: \tab Sergei Izrailev, Jeremy Stanley\cr
#' Maintainer: \tab Sergei Izrailev <sizrailev@@collective.com>\cr
#' LazyLoad: \tab yes\cr
#' }
#'
#' This package contains functions for fast loading CSV files,
#' and convenient timing functions. 
#' 
#' \code{\link{csvread}} is a CSV file loader that is much faster than the built-in read.csv, 
#' but needs column type specifications.
#' 
#' See also \code{\link{int64}} for information about dealing with 64-bit integers when loading data from 
#' CSV files. 
#' 
#' \code{\link{cm.tic}} provides the timing functions \code{tic} and \code{toc} that can be nested. 
#' One can record all timings while a complex script is running, and examine the values later.
#' 
#' A stack implemented as a vector (\code{\link{cm.stack}}) and as a list (\code{\link{cm.list}}) 
#' with push, pop, first, last and clear operations are implemented.
#' 
#' There are also a few other utilities listed under \code{link{cm.utils}}.
#' 
#' @name csvread
#' @aliases csvread
#' @docType package
#' @title Utilities for fast load of CSV files and timing. 
#' @author Sergei Izrailev, Jeremy Stanley
#' @keywords csv timing profiling bigint 64-bit
#' @useDynLib csvread
#' @exportPattern "*"
#' 
# The next and last line should be the word 'NULL'.
NULL
