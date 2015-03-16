.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tokenizers

Tokenizers
==========

Summary
-------

Groonga has tokenizer module that tokenizes text. It is used when
the following cases:

  * Indexing text

    .. figure:: /images/reference/tokenizers/used-when-indexing.png
       :align: center
       :width: 80%

       Tokenizer is used when indexing text.

  * Searching by query

    .. figure:: /images/reference/tokenizers/used-when-searching.png
       :align: center
       :width: 80%

       Tokenizer is used when searching by query.

Tokenizer is an important module for full-text search. You can change
trade-off between `precision and recall
<http://en.wikipedia.org/wiki/Precision_and_recall>`_ by changing
tokenizer.

Normally, :ref:`token-bigram` is a suitable tokenizer. If you don't
know much about tokenizer, it's recommended that you choose
:ref:`token-bigram`.

You can try a tokenizer by :doc:`/reference/commands/tokenize.rst` and
:doc:`/reference/commands/table_tokenize.rst`. Here is an example to
try :ref:`token-bigram` tokenizer by
:doc:`/reference/commands/tokenize.rst`:

.. groonga-command
.. include:: ../example/reference/tokenizers/tokenize-example.log
.. tokenize TokenBigram "Hello World"

What is "tokenize"?
-------------------

"tokenize" is the process that extracts zero or more tokens from a
text. There are some "tokenize" methods.

For example, ``Hello World`` is tokenized to the following tokens by
bigram tokenize method:

  * ``He``
  * ``el``
  * ``ll``
  * ``lo``
  * ``o ``
  * `` W``
  * ``Wo``
  * ``or``
  * ``rl``
  * ``ld``

In the above example, 10 tokens are extracted from one text ``Hello
World``.

For example, ``Hello World`` is tokenized to the following tokens by
white-space-separate tokenize method:

  * ``Hello``
  * ``World``

In the above example, 2 tokens are extracted from one text ``Hello
World``.

Token is used as search key. You can find indexed documents only by
tokens that are extracted by used tokenize method. For example, you
can find ``Hello World`` by ``ll`` with bigram tokenize method but you
can't find ``Hello World`` by ``ll`` with white-space-separate tokenize
method. Because white-space-separate tokenize method doesn't extract
``ll`` token. It just extracts ``Hello`` and ``World`` tokens.

In general, tokenize method that generates small tokens increases
recall but decreases precision. Tokenize method that generates large
tokens increases precision but decreases recall.

For example, we can find ``Hello World`` and ``A or B`` by ``or`` with
bigram tokenize method. ``Hello World`` is a noise for people who
wants to search "logical and". It means that precision is
decreased. But recall is increased.

We can find only ``A or B`` by ``or`` with white-space-separate
tokenize method. Because ``World`` is tokenized to one token ``World``
with white-space-separate tokenize method. It means that precision is
increased for people who wants to search "logical and". But recall is
decreased because ``Hello World`` that contains ``or`` isn't found.

Built-in tokenizsers
--------------------

Here is a list of built-in tokenizers:

  * ``TokenBigram``
  * ``TokenBigramSplitSymbol``
  * ``TokenBigramSplitSymbolAlpha``
  * ``TokenBigramSplitSymbolAlphaDigit``
  * ``TokenBigramIgnoreBlank``
  * ``TokenBigramIgnoreBlankSplitSymbol``
  * ``TokenBigramIgnoreBlankSplitAlpha``
  * ``TokenBigramIgnoreBlankSplitAlphaDigit``
  * ``TokenDelimit``
  * ``TokenDelimitNull``
  * ``TokenTrigram``
  * ``TokenUnigram``
  * ``TokenMecab``
  * ``TokenRegexp``

.. _token-bigram

``TokenBigram``
^^^^^^^^^^^^^^^

``TokenBigram`` is a bigram based tokenizer. It's recommended to use
this tokenizer for most cases.

``TokenBigram`` behavior is different when it's worked with any
:doc:/reference/normalizers .

If no normalizer is used, ``TokenBigram`` uses pure bigram (all tokens
except the last token have two characters) tokenize method:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-no-normalizer.log
.. tokenize TokenBigram "Hello World"

If normalizer is used, ``TokenBigram`` uses white-space-separate like
tokenize method for ASCII characters. ``TokenBigram`` uses bigram
tokenize method for non-ASCII characters.

``TokenBigram`` uses one or more white-spaces as token delimiter for
ASCII characters:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ascii-and-white-space-with-normalizer.log
.. tokenize TokenBigram "Hello World" NormalizerAuto

``TokenBigram`` uses character type change as token delimiter for
ASCII characters. Character type is one of them:

  * Alphabet
  * Digit
  * Symbol (such as ``(``, ``)`` and ``!``)
  * Hiragana
  * Katakana
  * Kanji
  * Others

The following example shows two token delimiters:

  * at between ``100`` (digits) and ``cents`` (alphabets)
  * at between ``cents`` (alphabets) and ``!!!`` (symbols)

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ascii-and-character-type-change-with-normalizer.log
.. tokenize TokenBigram "100cents!!!" NormalizerAuto

Here is an example that ``TokenBigram`` uses bigram tokenize method
for non-ASCII characters.

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-non-ascii-with-normalizer.log
.. tokenize TokenBigram "日本語の勉強" NormalizerAuto


.. _token-bigram-split-symbol

``TokenBigramSplitSymbol``
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigram-split-symbol-alpha

``TokenBigramSplitSymbolAlpha``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigram-split-symbol-alpha-digit

``TokenBigramSplitSymbolAlphaDigit``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigramIgnoreBlank

``TokenBigramIgnoreBlank``
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigramIgnoreBlank-split-symbol

``TokenBigramIgnoreBlankSplitSymbol``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigramIgnoreBlank-split-alpha

``TokenBigramIgnoreBlankSplitAlpha``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-bigramIgnoreBlank-split-alpha-digit

``TokenBigramIgnoreBlankSplitAlphaDigit``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _token-delimit

``TokenDelimit``
^^^^^^^^^^^^^^^^

.. _token-delimit-null

``TokenDelimitNull``
^^^^^^^^^^^^^^^^^^^^

.. _token-trigram

``TokenTrigram``
^^^^^^^^^^^^^^^^

.. _token-unigram

``TokenUnigram``
^^^^^^^^^^^^^^^^

.. _token-mecab

``TokenMecab``
^^^^^^^^^^^^^^

.. _token-regexp

``TokenRegexp``
^^^^^^^^^^^^^^^

.. versionadded:: 5.0.1

.. caution::

   This tokenizer is experimental. Specification may be changed.
