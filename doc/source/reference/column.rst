.. -*- rst -*-

Column
======

Column is a data store object or an index object for fast search.

A column belongs to a table. Table has zero or more columns.

Both data store column and index column have type. Type of data store
column specifies data range. In other words, it is "value type". Type
of index column specifies set of documents to be indexed. A set of
documents is a table in Groonga. In other words, type of index column
must be a table.

Here are data store columns:

.. toctree::
   :maxdepth: 1
   :glob:

   columns/scalar
   columns/vector
   columns/pseudo

Here is an index column:

.. toctree::
   :maxdepth: 1
   :glob:

   columns/index
