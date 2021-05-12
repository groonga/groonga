.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-ignore-blank-split-symbol-alpha-digit:

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``
===============================================

Summary
-------

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol, alphabet and digit handling

Syntax
------

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit`` hasn't parameter::

  TokenBigramIgnoreBlankSplitSymbolAlphaDigit

Usage
-----

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit`` ignores white-spaces
in continuous symbols and non-ASCII characters.

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit`` tokenizes symbols,
alphabets and digits by bigram tokenize method. It means that all
characters are tokenized by bigram tokenize method.

You can find difference of them by ``Hello 日 本 語 ! ! ! 777`` text
because it has symbols and non-ASCII characters with white spaces,
alphabets and digits.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol-and-alphabet-and-digit.log
.. tokenize TokenBigram "Hello 日 本 語 ! ! ! 777" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol-and-alphabet-digit.log
.. tokenize TokenBigramIgnoreBlankSplitSymbolAlphaDigit "Hello 日 本 語 ! ! ! 777" NormalizerAuto
