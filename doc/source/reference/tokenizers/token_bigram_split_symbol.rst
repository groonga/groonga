.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tokenizers

``TokenBigramSplitSymbol``
==========================

Summary
-------

``TokenBigramSplitSymbol`` is similar to :ref:`token-bigram`. The
difference between them is symbol handling.

Syntax
------

``TokenBigramSplitSymbol`` hasn't parameter::

  TokenBigramSplitSymbol

Usage
-----

``TokenBigramSplitSymbol`` tokenizes symbols by bigram tokenize method:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-split-symbol-with-normalizer.log
.. tokenize TokenBigramSplitSymbol "100cents!!!" NormalizerAuto
