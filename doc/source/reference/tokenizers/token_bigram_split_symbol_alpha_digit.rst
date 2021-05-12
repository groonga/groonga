.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-split-symbol-alpha-digit:

``TokenBigramSplitSymbolAlphaDigit``
====================================

Summary
-------

``TokenBigramSplitSymbolAlphaDigit`` is similar to
:ref:`token-bigram`. The difference between them is symbol, alphabet
and digit handling.

Syntax
------

``TokenBigramSplitSymbolAlphaDigit`` hasn't parameter::

  TokenBigramSplitSymbolAlphaDigit

Usage
-----

``TokenBigramSplitSymbolAlphaDigit`` tokenizes
symbols, alphabets and digits by bigram tokenize method. It means that
all characters are tokenized by bigram tokenize method:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-split-symbol-alpha-digit-with-normalizer.log
.. tokenize TokenBigramSplitSymbolAlphaDigit "100cents!!!" NormalizerAuto
