.. -*- rst -*-

.. groonga-command
.. database: tokenizers

``TokenPattern``
================

Summary
-------

``TokenPattern`` is a tokenizer which is used to extract tokens by regular expression.
This tokenizer extracts only token that matches the specified regular expression.

You can also specify multiple patterns of regular expression.

Syntax
------

``TokenPattern`` has optional parameter.

Specify one pattern::

  TokenPattern("pattern", PATTERN)

Specify multiple patterns::

  TokenPattern("pattern", PATTERN_1, "pattern", PATTERN_2, ... "pattern", PATTERN_N)


``TokenPattern`` can accept multiple patterns as above.

Usage
-----

Here is an example of ``TokenPattern``. As ``TokenPattern`` only extracts the token which matches the specified regular expression, it is able to filter search results which only matches the extracted token.

For example, let's compare search results by specific keywords. One is listed in ``TokenPattern`` pattern, and the other is not listed in ``TokenPattern`` pattern.

There are menus which contains both of specific keywords in ``Foods`` table.

Here is the sample schema and data:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-pattern-schema.log
.. table_create Foods TABLE_NO_KEY
.. column_create Foods name COLUMN_SCALAR Text
.. table_create Keywords TABLE_PAT_KEY ShortText --default_tokenizer 'TokenPattern("pattern", "Apple|Orange")'
.. column_create Keywords index COLUMN_INDEX Foods name
.. load --table Foods
.. [
.. {"name": "Apple Pie"},
.. {"name": "Orange Pie"}
.. {"name": "Raspberry Pie"}
.. ]


Then search ``Apple Pie`` with ``--query Apple``.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-pattern-search-apple.log
.. select Foods --match_columns name --query 'Apple'

In above example, ``Apple`` matches pattern which is specified as ``TokenPattern`` pattern, ``select`` matches ``Apple Pie``.

Then search ``Raspberry Pie`` with ``--query Raspberry``.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-pattern-search-raspberry.log
.. select Foods --match_columns name --query 'Raspberry'

In above example, even though ``Foods`` table contains ``Raspberry Pie`` record, ``select`` doesn't match it because ``Raspberry`` doesn't match to ``TokenPattern`` pattern..

See also
----------

* :doc:`../commands/tokenize`
