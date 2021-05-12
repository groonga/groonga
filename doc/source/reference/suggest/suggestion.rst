.. -*- rst -*-

.. groonga-command
.. % groonga-suggest-create-dataset /tmp/groonga-databases/suggestion query
.. database: suggestion

Suggestion
==========

This section describes about the following completion
features:

* How it works
* How to use
* How to learn

How it works
------------

The suggestion feature uses a search to compute suggested
words:

  1. Cooccurrence search against learned data.

Cooccurrence search
^^^^^^^^^^^^^^^^^^^

Cooccurrence search can find related words from user's
input. It uses user submissions that will be learned
from query logs, access logs and so on.

For example, there are the following user submissions:

+----------------------+
|  query               |
+======================+
| search engine        |
+----------------------+
| web search realtime  |
+----------------------+

Groonga creates the following suggestion pairs:

+----------+------------------------------------+
|  input   |          suggested words           |
+==========+====================================+
|search    |search engine                       |
+----------+------------------------------------+
|engine    |search engine                       |
+----------+------------------------------------+
|web       |web search realtime                 |
+----------+------------------------------------+
|search    |web search realtime                 |
+----------+------------------------------------+
|realtime  |web search realtime                 |
+----------+------------------------------------+

Those pairs are created by the following steps:

  1. Tokenizes user input query by TokenDelimit tokenizer
     that uses a space as token delimiter. (e.g. "search
     engine" is tokenized to two tokens "search" and
     "engine".)
  2. Creates a pair that is consists of a token and original
     query for each token.

If an user inputs "search" and cooccurrence search returns
"search engine" and "web search realtime" because "search" is
in two input columns and corresponding suggested word
columns have "search engine" and "web search realtime".

How to use
----------

.. groonga-command
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950803.86057, "item": "search engine", "type": "submit"},
.. {"sequence": "1", "time": 1312950808.86057, "item": "web search realtime", "type": "submit"}
.. ]

Groonga provides :doc:`/reference/commands/suggest` command to use
suggestion. `--type suggest` option requests suggestion

For example, here is an command to get suggestion results by
"search":

.. groonga-command
.. include:: ../../example/reference/suggest/suggest/select.log
.. suggest --table item_query --column kana --types suggest --frequency_threshold 1 --query search

How it learns
-------------

Cooccurrence search uses learned data. They are based on
query logs, access logs and so on. To create learned data,
groonga needs user input sequence with time stamp and user
submit input with time stamp.

For example, an user wants to search by "engine". The user
inputs the query with the following sequence:

  1. 2011-08-10T13:33:23+09:00: search engine (submit)
  2. 2011-08-10T13:33:28+09:00: web search realtime (submit)

Groonga can be learned from the submissions by the
following command::

  load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
  [
  {"sequence": "1", "time": 1312950803.86057, "item": "search engine", "type": "submit"},
  {"sequence": "1", "time": 1312950808.86057, "item": "web search realtime", "type": "submit"}
  ]

How to extract learning data
----------------------------

The learning data is stored into ``item_DATASET`` and ``pair_DATASET`` tables.
By using select command for such tables, you can all extract learing data.

Here is the query to extract all learning data::

  select item_DATASET --limit -1
  select pair_DATASET --filter 'freq0 > 0 || freq1 > 0 || freq2 > 0' --limit -1

Without '--limit -1', you can't get all data.
In pair table, the valid value of ``freq0``, ``freq1`` and ``freq2`` column must be larger than 0.

Don't execute above query via HTTP request because enormous number of records are fetched.
 
