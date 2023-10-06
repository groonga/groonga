.. -*- rst -*-

.. groonga-command
.. database: tokenizers_table

``TokenTable``
==============

Summary
-------

``TokenTable`` is a tokenizer which treats only known keywords as a token.
The known keywords must be registered as a key in the table for ``TokenTable``.

For example, query contains a known keyword and a unknown keyword, only a known keyword is
used in search query (unknown keyword will be ignored). In other words, you can search contents with only known keywords.
Because of this characteristic, you need to maintain keyword table for a new keyword continuously.

Syntax
------

``TokenTable`` has a required parameter.

Specify the table::

  TokenTable("table", TABLE)

``TABLE`` must be created with ``--default_tokenizer 'TokenTable("table", TABLE)``.

Usage
-----

Here is an example of ``TokenTable``.
For example, let's search ``Raspberry Pie`` from ``Pies`` table.
The table which is used for keyword is ``Keywords``.

Here is the sample schema and data:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-table-schema.log
.. table_create Pies TABLE_NO_KEY
.. column_create Pies name COLUMN_SCALAR Text
.. table_create Keywords TABLE_PAT_KEY ShortText --default_tokenizer 'TokenTable("table", "Keywords")'
.. column_create Keywords index COLUMN_INDEX Pies name
.. load --table Keywords
.. [
.. {"_key": "Apple"}
.. {"_key": "Orange"}
.. {"_key": "Raspberry"}
.. ]
.. load --table Pies
.. [
.. {"name": "Apple Pie"},
.. {"name": "Orange Pie"}
.. {"name": "Raspberry Pie"}
.. {"name": "Stargazy Pie"}
.. ]

Then search ``Raspberry Pie`` with ``--query Raspberry``.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-table-search-raspberry.log
.. select Pies --match_columns name --query 'Raspberry'

As you expected, the above query matches the ``Raspberry Pie`` record.

Next, search ``Stargazy Pie`` with ``--query Stargazy``.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-table-search-stargazy.log
.. select Pies --match_columns name --query 'Stargazy'

In above example, as the keyword ``Stargazy`` is not registered in ``Keywords`` table yet, it doesn't match anything.

See also
----------

* :doc:`../commands/tokenize`
