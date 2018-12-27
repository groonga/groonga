.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: token_filters_example

``TokenFilterNFKC100``
======================

Summary
-------

This token filter can translate a token for katakana to hiragana with same option of NormalizerNFKC100.
This token filter convenient when you want to get reading of token as hiragana.

Syntax
------

``TokenFilterNFKC100`` has a parameter::

  TokenFilterNFKC100("unify_kana", true)

Usage
-----

Simple usage
------------

Here is an example of ``TokenFilterNFKC100``. ``リンゴ`` is translated to ``りんご``.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100.log
.. tokenize TokenDelimit "リンゴ"   --token_filters 'TokenFilterNFKC100("unify_kana", true)'

``TokenFilterNFKC100`` is not translate a token of hiragana and kanji as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-hiragana-and-kanji.log
.. tokenize TokenDelimit "りんご 林檎"   --token_filters 'TokenFilterNFKC100("unify_kana", true)'

Advanced usage
--------------

You can output all input string as hiragana with cimbining ``TokenFilterNFKC100`` with ``use_reading`` option of ``TokenMecab`` as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-with-token-mecab.log
.. tokenize   'TokenMecab("use_reading", true)'   "私は林檎を食べます。"   --token_filters 'TokenFilterNFKC100("unify_kana", true)'

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are a required parameters ``unify_kana``.

``unify_kana``
""""""""""""""

Translate a token katakana to hiragana.
