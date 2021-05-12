.. -*- rst -*-

.. groonga-command
.. database: operations_prefix_rk_search

Prefix RK search
================

Summary
-------

Groonga supports prefix RK search. RK means Romaji and Kana (reading).
Prefix RK search can find registered text in katakana by query in
romaji, hiragana or katakana. Found registered texts are started with
query.

Prefix RK search is useful for completing Japanese text. Because
romaji is widely used to input Japanese on computer. See also
`Japanese input methods on Wikipedia
<https://en.wikipedia.org/wiki/Japanese_input_methods>`_.

If users can search Japanese text in romaji, users doesn't need to
convert romaji to hiragana, katakana or kanji by themselves. For
example, if you register a reading for "日本" as "ニホン", users can
find "日本" by "ni", "に" or "二".

The feature is helpful because it reduces one or more operations of
users.

This feature is used in :doc:`/reference/suggest/completion`.

You can use this feature in :ref:`select-filter` by
:doc:`/reference/functions/prefix_rk_search`.

Usage
-----

You need :ref:`table-pat-key` table for using prefix RK search.

You need to put reading in katakana to ``TABLE_PAT_KEY`` as key:

.. groonga-command
.. include:: ../../example/reference/operations/prefix_rk_search/usage_register_kana.log
.. table_create Readings TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
.. load --table Readings
.. [
.. {"_key": "ニホン"},
.. {"_key": "ニッポン"},
.. {"_key": "ローマジ"}
.. ]

You can finds ``ニホン`` and ``ニッポン`` by prefix RK search with
``ni`` as query from the ``Readings`` table.

You can finds ``ローマジ`` by prefix RK search with ``r`` as query
from the ``Readings`` table.

How to convert romaji to reading
--------------------------------

Prefix RK search is based on JIS X 4063:2000 specification.

The specification was obsoleted. See `ローマ字入力 on Japanese
Wikipedia
<https://ja.wikipedia.org/wiki/%E3%83%AD%E3%83%BC%E3%83%9E%E5%AD%97%E5%85%A5%E5%8A%9B>`_
for JIS X 4063:2000.

Normally, you can get converted results as expected.

See also
--------

  * :doc:`/reference/suggest/completion`
  * :doc:`/reference/functions/prefix_rk_search`
