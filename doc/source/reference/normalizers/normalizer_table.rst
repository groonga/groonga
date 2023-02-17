.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-table:

``NormalizerTable``
===================

Summary
-------

.. versionadded:: 11.0.4

``NormalizerTable`` normalizes text by user defined normalization table. User defined normalization table is just a normal table but it must satisfy some conditions. They are described later.

.. note::

   The normalized text is depends on contents of user defined
   normalization table. If you want to use this normalizer for
   lexicon, you need to re-index when you change your user defined
   normalization table.

Syntax
------

There are required and optional parameters.

Required parameters::

  NormalizerTable("normalized", "UserDefinedTable.normalized_column")

Optional parameters::

  NormalizerTable("normalized", "UserDefinedTable.normalized_column",
                  "target", "target_column")

  NormalizerTable("normalized", "UserDefinedTable.normalized_column",
                  "unicode_version", "13.0.0")

Usage
-----

.. _normalizer-table-simple-usage:

Simple usage
^^^^^^^^^^^^

Here is an example of ``NormalizerTable``.

``NormalizerTable`` normalizes text by user defined normalization table. You use the following user defined normalization table here:

  * Table type must be ``TABLE_PAT_KEY``.

  * Table key type must be ``ShortText``.

  * Table must have at least one ``ShortText`` column.

Here are schema and data for this example:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-table-simple-usage-prepare.log
.. table_create Normalizations TABLE_PAT_KEY ShortText
.. column_create Normalizations normalized COLUMN_SCALAR ShortText
.. load --table Normalizations
.. [
.. {"_key": "a", "normalized": "<A>"},
.. {"_key": "ac", "normalized": "<AC>"}
.. ]

You can normalize ``a`` with ``<A>`` and ``ac`` with ``<AC>`` with this user defined normalization table. For example:

  * ``Groonga`` -> ``Groong<A>``

  * ``hack`` -> ``h<AC>k``

Here are examples of ``NormalizerTable`` with the user defined normalization table:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-table-simple-usage-output.log
.. normalize 'NormalizerTable("normalized", "Normalizations.normalized")' "Groonga"
.. normalize 'NormalizerTable("normalized", "Normalizations.normalized")' "hack"

.. _normalizer-table-usage-unicode-version:

Unicode version
^^^^^^^^^^^^^^^

Some internal processings such as tokenization and highlight use character type. ``NormalizerTable`` provides character type based on Unicode. You can specify used Unicode version by :ref:`normalizer-table-unicode-version` option.

Here is an example to use Unicode 13.0.0:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-table-simple-usage-unicode-version.log
.. normalize 'NormalizerTable("normalized", "Normalizations.normalized")' "Groonga" WITH_TYPES

The default Unicode version is 5.0.0.

.. _normalizer-table-advanced-usage:

Advanced usage
^^^^^^^^^^^^^^

You can put a normalized string to a column instead of ``_key``. In this case, you need to create the following index column for the column:

  * Lexicon type of the index column must be ``TABLE_PAT_KEY``.

  * Lexicon key type of the index column must be ``ShortText``.

  * Lexicon of the index column must not have tokenizer.

You can use any table type for this usage such as ``TABLE_NO_KEY``. This is useful when you can't control table type. For example, PGroonga users can only use this usage.

Here are schema and data for this example:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-table-advanced-usage-prepare.log
.. table_create ColumnNormalizations TABLE_NO_KEY
.. column_create ColumnNormalizations target_column COLUMN_SCALAR ShortText
.. column_create ColumnNormalizations normalized COLUMN_SCALAR ShortText
..
.. table_create Targets TABLE_PAT_KEY ShortText
.. column_create Targets column_normalizations_target_column \
..    COLUMN_INDEX ColumnNormalizations target_column
..
.. load --table ColumnNormalizations
.. [
.. {"target_column": "a", "normalized": "<A>"},
.. {"target_column": "ac", "normalized": "<AC>"}
.. ]

You need to use :ref:`normalizer-table-target` option to use the user defined normalization table. The above schema uses ``target_column`` for explanation. Generally, ``_column`` in ``target_column`` is redundant but it's added for easy to distinct parameter name and parameter value.

Here are examples of ``NormalizerTable`` with the user defined normalization table:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-table-simple-usage-output.log
.. normalize 'NormalizerTable("normalized", "ColumnNormalizations.normalized", "target", "target_column")' "Groonga"
.. normalize 'NormalizerTable("normalized", "ColumnNormalizations.normalized", "target", "target_column")' "hack"

Parameters
----------

Required parameter
^^^^^^^^^^^^^^^^^^

.. _normalizer-table-normalized:

``normalized``
""""""""""""""

This option specifies a column that has normalized texts. Normalized target texts are texts in corresponding ``_key`` column or column specified by :ref:`normalizer-table-target`.

Value type of the column specified for this option must be one of ``ShortText``, ``Text`` and ``LongText``.

If you don't use :ref:`normalizer-table-target`, the table of column specified for this option must satisfy the followings:

  * Table type is ``TABLE_PAT_KEY``

  * Table key type is ``ShortText``

See :ref:`normalizer-table-simple-usage` for usage of this case.

Optional parameters
^^^^^^^^^^^^^^^^^^^

.. _normalizer-table-target:

``target``
""""""""""

This option specifies a column that has normalization target texts.

Value type of the column specified for this option must be one of ``ShortText``, ``Text`` and ``LongText``.

You must create an index column for the column specified for this option. The index column and its lexicon must satisfies the followings:

  * Index column can be a single column index or a multi column index.

  * Lexicon type of the index column must be ``TABLE_PAT_KEY``.

  * Lexicon key type of the index column must be ``ShortText``.

  * Lexicon of the index must not have tokenizer.

See :ref:`normalizer-table-advanced-usage` for usage of this case.

.. _normalizer-table-unicode-version:

``unicode_version``
"""""""""""""""""""

This option specifies Unicode version to use determining character type.

The default Unicode version is 5.0.0.

See :ref:`normalizer-table-usage-unicode-version` for usage.

See also
--------

* :doc:`../commands/normalize`
