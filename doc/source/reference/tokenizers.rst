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

You can try a tokenizer by :doc:`/reference/commands/tokenize` and
:doc:`/reference/commands/table_tokenize`. Here is an example to
try :ref:`token-bigram` tokenizer by
:doc:`/reference/commands/tokenize`:

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
  * ``o_`` (``_`` means a white-space)
  * ``_W`` (``_`` means a white-space)
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
  * ``TokenBigramIgnoreBlankSplitSymbolAlpha``
  * ``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``
  * ``TokenUnigram``
  * ``TokenTrigram``
  * ``TokenDelimit``
  * ``TokenDelimitNull``
  * ``TokenMecab``
  * ``TokenRegexp``

.. toctree::
   :maxdepth: 1
   :glob:

   tokenizers/*

.. _token-bigram-ignore-blank-split-symbol:

``TokenBigramIgnoreBlankSplitSymbol``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramIgnoreBlankSplitSymbol`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol handling

``TokenBigramIgnoreBlankSplitSymbol`` ignores white-spaces in
continuous symbols and non-ASCII characters.

``TokenBigramIgnoreBlankSplitSymbol`` tokenizes symbols by bigram
tokenize method.

You can find difference of them by ``日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol.log
.. tokenize TokenBigram "日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbol``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol.log
.. tokenize TokenBigramIgnoreBlankSplitSymbol "日 本 語 ! ! !" NormalizerAuto

.. _token-bigram-ignore-blank-split-symbol-alpha:

``TokenBigramIgnoreBlankSplitSymbolAlpha``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramIgnoreBlankSplitSymbolAlpha`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol and alphabet handling

``TokenBigramIgnoreBlankSplitSymbolAlpha`` ignores white-spaces in
continuous symbols and non-ASCII characters.

``TokenBigramIgnoreBlankSplitSymbolAlpha`` tokenizes symbols and
alphabets by bigram tokenize method.

You can find difference of them by ``Hello 日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters with white spaces and alphabets.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol-and-alphabet.log
.. tokenize TokenBigram "Hello 日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbolAlpha``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol-and-alphabet.log
.. tokenize TokenBigramIgnoreBlankSplitSymbolAlpha "Hello 日 本 語 ! ! !" NormalizerAuto

.. _token-bigram-ignore-blank-split-symbol-alpha-digit:

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramIgnoreBlankSplitSymbolAlphaDigit`` is similar to
:ref:`token-bigram`. The differences between them are the followings:

  * Blank handling
  * Symbol, alphabet and digit handling

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
.. include:: ../example/reference/tokenizers/token-bigram-with-white-spaces-and-symbol-and-alphabet-and-digit.log
.. tokenize TokenBigram "Hello 日 本 語 ! ! ! 777" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ignore-blank-split-symbol-with-white-spaces-and-symbol-and-alphabet-digit.log
.. tokenize TokenBigramIgnoreBlankSplitSymbolAlphaDigit "Hello 日 本 語 ! ! ! 777" NormalizerAuto

.. _token-unigram:

``TokenUnigram``
^^^^^^^^^^^^^^^^

``TokenUnigram`` is similar to :ref:`token-bigram`. The differences
between them is token unit. :ref:`token-bigram` uses 2 characters per
token. ``TokenUnigram`` uses 1 character per token.

.. groonga-command
.. include:: ../example/reference/tokenizers/token-unigram.log
.. tokenize TokenUnigram "100cents!!!" NormalizerAuto

.. _token-trigram:

``TokenTrigram``
^^^^^^^^^^^^^^^^

``TokenTrigram`` is similar to :ref:`token-bigram`. The differences
between them is token unit. :ref:`token-bigram` uses 2 characters per
token. ``TokenTrigram`` uses 3 characters per token.

.. groonga-command
.. include:: ../example/reference/tokenizers/token-trigram.log
.. tokenize TokenTrigram "10000cents!!!!!" NormalizerAuto

.. _token-delimit-null:

``TokenDelimitNull``
^^^^^^^^^^^^^^^^^^^^

``TokenDelimitNull`` is similar to :ref:`token-delimit`. The
difference between them is separator character. :ref:`token-delimit`
uses space character (``U+0020``) but ``TokenDelimitNull`` uses NUL
character (``U+0000``).

``TokenDelimitNull`` is also suitable for tag text.

Here is an example of ``TokenDelimitNull``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-delimit-null.log
.. tokenize TokenDelimitNull "Groonga\u0000full-text-search\u0000HTTP" NormalizerAuto

.. _token-regexp:

``TokenRegexp``
^^^^^^^^^^^^^^^

.. versionadded:: 5.0.1

.. caution::

   This tokenizer is experimental. Specification may be changed.

.. caution::

   This tokenizer can be used only with UTF-8. You can't use this
   tokenizer with EUC-JP, Shift_JIS and so on.

``TokenRegexp`` is a tokenizer for supporting regular expression
search by index.

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
.. include:: ../example/reference/tokenizers/token-regexp-add.log
.. tokenize TokenRegexp "/home/alice/test.txt" NormalizerAuto --mode ADD
