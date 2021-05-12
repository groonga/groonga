.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-split-symbol-alpha:

``TokenBigramSplitSymbolAlpha``
===============================

Summary
-------

``TokenBigramSplitSymbolAlpha`` is similar to :ref:`token-bigram`. The
difference between them is symbol and alphabet handling.

Syntax
------

``TokenBigramSplitSymbolAlpha`` hasn't parameter::

  TokenBigramSplitSymbolAlpha

Usage
-----

``TokenBigramSplitSymbolAlpha`` tokenizes symbols and
alphabets by bigram tokenize method:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-split-symbol-alpha-with-normalizer.log
.. tokenize TokenBigramSplitSymbolAlpha "100cents!!!" NormalizerAuto
