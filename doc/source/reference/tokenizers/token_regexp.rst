.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-regexp:

``TokenRegexp``
===============

Summary
-------

.. versionadded:: 5.0.1

.. caution::

   This tokenizer is experimental. Specification may be changed.

.. caution::

   This tokenizer can be used only with UTF-8. You can't use this
   tokenizer with EUC-JP, Shift_JIS and so on.

``TokenRegexp`` is a tokenizer for supporting regular expression
search by index.

Syntax
------

``TokenRegexp`` hasn't parameter::

  TokenRegexp

Usage
-----

In general, regular expression search is evaluated as sequential
search. But the following cases can be evaluated as index search:

  * Literal only case such as ``hello``
  * The beginning of text and literal case such as ``\A/home/alice``
  * The end of text and literal case such as ``\.txt\z``

In most cases, index search is faster than sequential search.

``TokenRegexp`` is based on bigram tokenize method. ``TokenRegexp``
adds the beginning of text mark (``U+FFEF``) at the begging of text
and the end of text mark (``U+FFF0``) to the end of text when you
index text:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-regexp-add.log
.. tokenize TokenRegexp "/home/alice/test.txt" NormalizerAuto --mode ADD
