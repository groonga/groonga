.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-ignore-blank-split-symbol-alpha:

``TokenBigramIgnoreBlankSplitSymbolAlpha``
==========================================

Summary
-------

``TokenBigramIgnoreBlankSplitSymbolAlpha`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol and alphabet handling

Syntax
------

``TokenBigramIgnoreBlankSplitSymbolAlpha`` hasn't parameter::

  TokenBigramIgnoreBlankSplitSymbolAlpha

Usage
-----

``TokenBigramIgnoreBlankSplitSymbolAlpha`` ignores white-spaces in
continuous symbols and non-ASCII characters.

``TokenBigramIgnoreBlankSplitSymbolAlpha`` tokenizes symbols and
alphabets by bigram tokenize method.

You can find difference of them by ``Hello 日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters with white spaces and alphabets.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol-and-alphabet.log
.. tokenize TokenBigram "Hello 日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbolAlpha``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol-and-alphabet.log
.. tokenize TokenBigramIgnoreBlankSplitSymbolAlpha "Hello 日 本 語 ! ! !" NormalizerAuto
