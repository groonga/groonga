.. -*- rst -*-

.. groonga-command
.. database: correction
.. $ groonga-suggest-create-dataset ${DB_PATH} query

Correction
==========

This section describes about the following correction
features:

* How it works
* How to use
* How to learn

How it works
------------

The correction feature uses three searches to compute corrected
words:

  1. Cooccurrence search against learned data.
  2. Similar search against registered words. (optional)

Cooccurrence search
^^^^^^^^^^^^^^^^^^^

Cooccurrence search can find registered words from user's
wrong input. It uses user submit sequences that will be
learned from query logs, access logs and so on.

For example, there are the following user submissions:

+-------------------+---------------------------+
|  query            |    time                   |
+===================+===========================+
| serach (typo!)    | 2011-08-10T22:20:50+09:00 |
+-------------------+---------------------------+
| search (fixed!)   | 2011-08-10T22:20:52+09:00 |
+-------------------+---------------------------+

Groonga creates the following correction pair from the above
submissions:

+----------+--------------------+
|  input   |   corrected word   |
+==========+====================+
|serach    |search              |
+----------+--------------------+

Groonga treats continuous submissions within a minute as
input correction by user. Not submitted user input sequence
between two submissions isn't used as learned data for
correction.

If an user inputs "serach" and cooccurrence search returns
"search" because "serach" is in input column and
corresponding corrected word column value is "search".

Similar search
^^^^^^^^^^^^^^

Similar search can find registered words that has one or
more the same tokens as user input. TokenBigram tokenizer is
used for tokenization because suggest dataset schema
created by :doc:`/reference/executables/groonga-suggest-create-dataset`
uses TokenBigram tokenizer as the default tokenizer.

For example, there is a registered query "search engine". An
user can find "search engine" by "web search service",
"sound engine" and so on. Because "search engine" and "web
search engine" have the same token "search" and "search
engine" and "sound engine" have the same token "engine".

"search engine" is tokenized to "search" and "engine"
tokens. (Groonga's TokenBigram tokenizer doesn't tokenize
two characters for continuous alphabets and continuous
digits for reducing search
noise. TokenBigramSplitSymbolAlphaDigit tokenizer should be
used to ensure tokenizing to two characters.) "web search
service" is tokenized to "web", "search" and
"service". "sound engine" is tokenized to "sound" and
"engine".

How to use
----------

.. groonga-command
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950803.86057, "item": "s"},
.. {"sequence": "1", "time": 1312950803.96857, "item": "sa"},
.. {"sequence": "1", "time": 1312950804.26057, "item": "sae"},
.. {"sequence": "1", "time": 1312950804.56057, "item": "saer"},
.. {"sequence": "1", "time": 1312950804.76057, "item": "saerc"},
.. {"sequence": "1", "time": 1312950805.76057, "item": "saerch", "type": "submit"},
.. {"sequence": "1", "time": 1312950809.76057, "item": "serch"},
.. {"sequence": "1", "time": 1312950810.86057, "item": "search", "type": "submit"}
.. ]

Groonga provides :doc:`/reference/commands/suggest` command to use
correction. `--type correct` option requests corrections.

For example, here is an command to get correction results by
"saerch":

.. groonga-command
.. include:: ../../example/reference/suggest/correction/select.log
.. suggest --table item_query --column kana --types correction --frequency_threshold 1 --query saerch

How it learns
-------------

Cooccurrence search uses learned data. They are based on
query logs, access logs and so on. To create learned data,
groonga needs user submit inputs with time stamp.

For example, an user wants to search by "search" but the
user has typo "saerch" before inputs the correct query. The
user inputs the query with the following sequence:

  1. 2011-08-10T13:33:23+09:00: s
  2. 2011-08-10T13:33:23+09:00: sa
  3. 2011-08-10T13:33:24+09:00: sae
  4. 2011-08-10T13:33:24+09:00: saer
  5. 2011-08-10T13:33:24+09:00: saerc
  6. 2011-08-10T13:33:25+09:00: saerch (submit!)
  7. 2011-08-10T13:33:29+09:00: serch (correcting...)
  8. 2011-08-10T13:33:30+09:00: search (submit!)

Groonga can be learned from the input sequence by the
following command::

  load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
  [
  {"sequence": "1", "time": 1312950803.86057, "item": "s"},
  {"sequence": "1", "time": 1312950803.96857, "item": "sa"},
  {"sequence": "1", "time": 1312950804.26057, "item": "sae"},
  {"sequence": "1", "time": 1312950804.56057, "item": "saer"},
  {"sequence": "1", "time": 1312950804.76057, "item": "saerc"},
  {"sequence": "1", "time": 1312950805.76057, "item": "saerch", "type": "submit"},
  {"sequence": "1", "time": 1312950809.76057, "item": "serch"},
  {"sequence": "1", "time": 1312950810.86057, "item": "search", "type": "submit"}
  ]

