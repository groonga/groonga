.. -*- rst -*-

.. groonga-command
.. database: tutorial-query-expansion

Query expansion
===============

Groonga accepts ``query_expander`` parameter for :doc:`/reference/commands/select` command.
It enables you to extend your query string.

For example, if user searches "theatre" instead of "theater",
query expansion enables to return search results of "theatre OR theater".
This kind of way reduces search leakages. This is what really user wants.

Preparation
-----------

To use query expansion, you need to create table which stores documents, synonym table which stores query string and replacement string.
In synonym table, primary key represents original string, the column of ShortText represents modified string.

.. TODO: 文字列型のベクターカラムでも可能であり、その場合は各要素をORでつなげたものに置換されるということを記述する。

Let's create document table and synonym table.

.. groonga-command
.. include:: ../example/tutorial/query_expansion-1.log
.. table_create Doc TABLE_PAT_KEY ShortText
.. column_create Doc body COLUMN_SCALAR ShortText
.. table_create Term TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Term Doc_body COLUMN_INDEX|WITH_POSITION Doc body
.. table_create Synonym TABLE_PAT_KEY ShortText
.. column_create Synonym body COLUMN_VECTOR ShortText
.. load --table Doc
.. [
.. {"_key": "001", "body": "Play all night in this theater."},
.. {"_key": "002", "body": "theatre is British spelling."},
.. ]
.. load --table Synonym
.. [
.. {"_key": "theater", "body": ["theater", "theatre"]},
.. {"_key": "theatre", "body": ["theater", "theatre"]},
.. ]

In this case, it doesn't occur search leakage because it creates synonym table which accepts "theatre" and "theater" as query string.

Search
------

Then, let's use prepared synonym table.
First, use select command without ``query_expander`` parameter.

.. groonga-command
.. include:: ../example/tutorial/query_expansion-2.log
.. select Doc --match_columns body --query "theater"
.. select Doc --match_columns body --query "theatre"

Above query returns the record which completely equal to query string.

Then, use ``query_expander`` parameter against ``body`` column of ``Synonym`` table.

.. groonga-command
.. include:: ../example/tutorial/query_expansion-3.log
.. select Doc --match_columns body --query "theater" --query_expander Synonym.body
.. select Doc --match_columns body --query "theatre" --query_expander Synonym.body

In which cases, query string is replaced to "(theater OR theatre)", thus synonym is considered for full text search.
