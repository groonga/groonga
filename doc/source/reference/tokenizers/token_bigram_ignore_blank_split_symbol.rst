.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-ignore-blank-split-symbol:

``TokenBigramIgnoreBlankSplitSymbol``
=====================================

Summary
-------

``TokenBigramIgnoreBlankSplitSymbol`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol handling

Syntax
------

``TokenBigramIgnoreBlankSplitSymbol`` hasn't parameter::

  TokenBigramIgnoreBlankSplitSymbol

Usage
-----

``TokenBigramIgnoreBlankSplitSymbol`` ignores white-spaces in
continuous symbols and non-ASCII characters.

``TokenBigramIgnoreBlankSplitSymbol`` tokenizes symbols by bigram
tokenize method.

You can find difference of them by ``日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol.log
.. tokenize TokenBigram "日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbol``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol.log
.. tokenize TokenBigramIgnoreBlankSplitSymbol "日 本 語 ! ! !" NormalizerAuto
