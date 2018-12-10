csvread
========

This R package provides functions for loading large (10M+ lines) CSV
    and other delimited files, similar to read.csv, but typically faster
    and using less memory than the standard R loader. While not entirely general,
    it covers many common use cases when the types of columns in the CSV file are known
    in advance. In addition, the package provides a class 'int64', which
    represents 64-bit integers exactly when reading from a file.
    The latter is useful when working with 64-bit integer identifiers
    exported from databases.

See <a href="http://jabiru.github.io/csvread/">http://jabiru.github.io/csvread/</a> for more details.
