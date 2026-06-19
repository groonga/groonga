.. -*- rst -*-

Column
======

Summary
-------

Column is a data store object or an index object for fast search.

A column belongs to a table. Table has zero or more columns.

Both data store column and index column have type. Type of data store
column specifies data range. In other words, it is "value type". Type
of index column specifies set of documents to be indexed. A set of
documents is a table in Groonga. In other words, type of index column
must be a table.

Data store columns
------------------

.. toctree::
   :maxdepth: 1
   :glob:

   columns/scalar
   columns/vector
   columns/pseudo

Index column
------------

.. toctree::
   :maxdepth: 1
   :glob:

   columns/index

.. _generated-column:

Generated column
----------------

You can generate contents of a data store column from other column's
value automatically. It's called as a generated column.

:doc:`columns/vector` and :doc:`columns/scalar` can be a generated
column. You can use :ref:`column-create-generator` to create a
generated column. See :ref:`column-create-generated-column` for
details.
