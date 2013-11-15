.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. % groonga-suggest-create-dataset /tmp/groonga-databases/completion query
.. database: completion

Completion
==========

This section describes about the following completion
features:

* How it works
* How to use
* How to learn

How it works
------------

The completion feature uses three searches to compute
completed words:

  1. Prefix RK search against registered words.
  2. Cooccurrence search against learned data.
  3. Prefix search against registered words. (optional)

Prefix RK search
^^^^^^^^^^^^^^^^

RK means Romaji and Katakana. Prefix RK search can find
registered words that start with user's input by romaji,
katakana or hiragana. It's useful for searching in Japanese.

For example, there is a registered word "日本". And "ニホン"
(it must be katakana) is registered as its reading. An user
can find "日本" by "ni", "二" or "に".

If you create dataset which is named as example by
:doc:`/reference/executables/groonga-suggest-create-dataset` command,
you can update pairs of registered word and its reading by loading
data to '_key' and 'kana' column of item_example table explicitly for
prefix RK search.

Cooccurrence search
^^^^^^^^^^^^^^^^^^^

Cooccurrence search can find registered words from user's
partial input. It uses user input sequences that will be
learned from query logs, access logs and so on.

For example, there is the following user input sequence:

+----------+----------+
|  input   |  submit  |
+==========+==========+
|s         |no        |
+----------+----------+
|se        |no        |
+----------+----------+
|sea       |no        |
+----------+----------+
|sear      |no        |
+----------+----------+
|searc     |no        |
+----------+----------+
|search    |yes       |
+----------+----------+
|e         |no        |
+----------+----------+
|en        |no        |
+----------+----------+
|eng       |no        |
+----------+----------+
|engi      |no        |
+----------+----------+
|engin     |no        |
+----------+----------+
|engine    |no        |
+----------+----------+
|enginen   |no (typo!)|
+----------+----------+
|engine    |yes       |
+----------+----------+

Groonga creates the following completion pairs:

+----------+--------------------+
|  input   |   completed word   |
+==========+====================+
|s         |search              |
+----------+--------------------+
|se        |search              |
+----------+--------------------+
|sea       |search              |
+----------+--------------------+
|sear      |search              |
+----------+--------------------+
|searc     |search              |
+----------+--------------------+
|e         |engine              |
+----------+--------------------+
|en        |engine              |
+----------+--------------------+
|eng       |engine              |
+----------+--------------------+
|engi      |engine              |
+----------+--------------------+
|engin     |engine              |
+----------+--------------------+
|engine    |engine              |
+----------+--------------------+
|enginen   |engine              |
+----------+--------------------+

All user not-submitted inputs (e.g. "s", "se" and so on)
before each an user submission maps to the submitted input
(e.g. "search").

To be precise, this description isn't correct because it
omits about time stamp. Groonga doesn't case about "all user
not-submitted inputs before each an user
submission". Groonga just case about "all user not-submitted
inputs within a minute from an user submission before each
an user submission". Groonga doesn't treat user inputs
before a minute ago.

If an user inputs "sea" and cooccurrence search returns
"search" because "sea" is in input column and corresponding
completed word column value is "search".

Prefix search
^^^^^^^^^^^^^

Prefix search can find registered word that start with
user's input. This search doesn't care about romaji,
katakana and hiragana not like Prefix RK search.

This search isn't always ran. It's just ran when it's
requested explicitly or both prefix RK search and
cooccurrence search return nothing.

For example, there is a registered word "search". An user
can find "search" by "s", "se", "sea", "sear", "searc" and
"search".

How to use
----------

.. groonga-command
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950803.86057, "item": "e"},
.. {"sequence": "1", "time": 1312950803.96857, "item": "en"},
.. {"sequence": "1", "time": 1312950804.26057, "item": "eng"},
.. {"sequence": "1", "time": 1312950804.56057, "item": "engi"},
.. {"sequence": "1", "time": 1312950804.76057, "item": "engin"},
.. {"sequence": "1", "time": 1312950805.86057, "item": "engine", "type": "submit"}
.. ]

Groonga provides :doc:`/reference/commands/suggest` command to use
completion. `--type complete` option requests completion.

For example, here is an command to get completion results by
"en":

.. groonga-command
.. include:: ../example/completion-1.log
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query en

How it learns
-------------

Cooccurrence search uses learned data. They are based on
query logs, access logs and so on. To create learned data,
groonga needs user input sequence with time stamp and user
submit input with time stamp.

For example, an user wants to search by "engine". The user
inputs the query with the following sequence:

  1. 2011-08-10T13:33:23+09:00: e
  2. 2011-08-10T13:33:23+09:00: en
  3. 2011-08-10T13:33:24+09:00: eng
  4. 2011-08-10T13:33:24+09:00: engi
  5. 2011-08-10T13:33:24+09:00: engin
  6. 2011-08-10T13:33:25+09:00: engine (submit!)

Groonga can be learned from the input sequence by the
following command::

  load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
  [
  {"sequence": "1", "time": 1312950803.86057, "item": "e"},
  {"sequence": "1", "time": 1312950803.96857, "item": "en"},
  {"sequence": "1", "time": 1312950804.26057, "item": "eng"},
  {"sequence": "1", "time": 1312950804.56057, "item": "engi"},
  {"sequence": "1", "time": 1312950804.76057, "item": "engin"},
  {"sequence": "1", "time": 1312950805.86057, "item": "engine", "type": "submit"}
  ]

How to update RK reading data
-----------------------------

Groonga requires registered word and its reading for RK search, so load such data in the advance.


Here is the example to register "日本" which means Japanese in english.

.. groonga-command
.. include:: ../example/suggest/complete/registered-word-japan.log
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950805.86058, "item": "日本", "type": "submit"}
.. ]


Here is the example to update RK data to complete "日本".

.. groonga-command
.. include:: ../example/suggest/complete/update-rk-data.log
.. load --table item_query
.. [
.. {"_key":"日本", "kana":["ニホン", "ニッポン"]}
.. ]

Then you can complete registered word "日本" by RK input - "nihon".

.. groonga-command
.. include:: ../example/suggest/complete/rk-search-nihon.log
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query nihon

Without loading above RK data, you can't complete registered word "日本"
by query - "nihon".

As the column type of item_query table is VECTOR_COLUMN, you can
register multiple readings for registered word.

This is the reason that you can also complete the registered word "日本"
by query - "nippon".

.. groonga-command
.. include:: ../example/suggest/complete/rk-search-nippon.log
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query nippon

This feature is very convenient because you can search registered word
even though Japanese IM is disabled.

If there are multiple candidates as completed result, you can
customize priority to set the value of "boost" column in item_query
table.

Here is the example to customize priority for RK search.

.. groonga-command
.. include:: ../example/suggest/complete/registered-word-japanese.log
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950805.86059, "item": "日本語", "type": "submit"}
.. {"sequence": "1", "time": 1312950805.86060, "item": "日本人", "type": "submit"}
.. ]
.. load --table item_query
.. [
.. {"_key":"日本語", "kana":"ニホンゴ"}
.. {"_key":"日本人", "kana":"ニホンジン"}
.. ]
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query nihon
.. load --table item_query
.. [
.. {"_key":"日本人", "boost": 100},
.. ]
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query nihon

