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

.. _token-bigram:

``TokenBigram``
^^^^^^^^^^^^^^^

``TokenBigram`` is a bigram based tokenizer. It's recommended to use
this tokenizer for most cases.

Bigram tokenize method tokenizes a text to two adjacent characters
tokens. For example, ``Hello`` is tokenized to the following tokens:

  * ``He``
  * ``el``
  * ``ll``
  * ``lo``

Bigram tokenize method is good for recall because you can find all
texts by query consists of two or more characters.

In general, you can't find all texts by query consists of one
character because one character token doesn't exist. But you can find
all texts by query consists of one character in Groonga. Because
Groonga find tokens that start with query by predictive search. For
example, Groonga can find ``ll`` and ``lo`` tokens by ``l`` query.

Bigram tokenize method isn't good for precision because you can find
texts that includes query in word. For example, you can find ``world``
by ``or``. This is more sensitive for ASCII only languages rather than
non-ASCII languages. ``TokenBigram`` has solution for this problem
described in the below.

``TokenBigram`` behavior is different when it's worked with any
:doc:`/reference/normalizers`.

If no normalizer is used, ``TokenBigram`` uses pure bigram (all tokens
except the last token have two characters) tokenize method:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-no-normalizer.log
.. tokenize TokenBigram "Hello World"

If normalizer is used, ``TokenBigram`` uses white-space-separate like
tokenize method for ASCII characters. ``TokenBigram`` uses bigram
tokenize method for non-ASCII characters.

You may be confused with this combined behavior. But it's reasonable
for most use cases such as English text (only ASCII characters) and
Japanese text (ASCII and non-ASCII characters are mixed).

Most languages consists of only ASCII characters use white-space for
word separator. White-space-separate tokenize method is suitable for
the case.

Languages consists of non-ASCII characters don't use white-space for
word separator. Bigram tokenize method is suitable for the case.

Mixed tokenize method is suitable for mixed language case.

If you want to use bigram tokenize method for ASCII character, see
``TokenBigramSplitXXX`` type tokenizers such as
:ref:`token-bigram-split-symbol-alpha`.

Let's confirm ``TokenBigram`` behavior by example.

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

.. _token-bigram-split-symbol:

``TokenBigramSplitSymbol``
^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramSplitSymbol`` is similar to :ref:`token-bigram`. The
difference between them is symbol handling. ``TokenBigramSplitSymbol``
tokenizes symbols by bigram tokenize method:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-split-symbol-with-normalizer.log
.. tokenize TokenBigramSplitSymbol "100cents!!!" NormalizerAuto

.. _token-bigram-split-symbol-alpha:

``TokenBigramSplitSymbolAlpha``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramSplitSymbolAlpha`` is similar to :ref:`token-bigram`. The
difference between them is symbol and alphabet
handling. ``TokenBigramSplitSymbolAlpha`` tokenizes symbols and
alphabets by bigram tokenize method:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-split-symbol-alpha-with-normalizer.log
.. tokenize TokenBigramSplitSymbolAlpha "100cents!!!" NormalizerAuto

.. _token-bigram-split-symbol-alpha-digit:

``TokenBigramSplitSymbolAlphaDigit``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramSplitSymbolAlphaDigit`` is similar to
:ref:`token-bigram`. The difference between them is symbol, alphabet
and digit handling. ``TokenBigramSplitSymbolAlphaDigit`` tokenizes
symbols, alphabets and digits by bigram tokenize method. It means that
all characters are tokenized by bigram tokenize method:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-split-symbol-alpha-digit-with-normalizer.log
.. tokenize TokenBigramSplitSymbolAlphaDigit "100cents!!!" NormalizerAuto

.. _token-bigram-ignore-blank:

``TokenBigramIgnoreBlank``
^^^^^^^^^^^^^^^^^^^^^^^^^^

``TokenBigramIgnoreBlank`` is similar to :ref:`token-bigram`. The
difference between them is blank handling. ``TokenBigramIgnoreBlank``
ignores white-spaces in continuous symbols and non-ASCII characters.

You can find difference of them by ``日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-with-white-spaces.log
.. tokenize TokenBigram "日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlank``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-bigram-ignore-blank-with-white-spaces.log
.. tokenize TokenBigramIgnoreBlank "日 本 語 ! ! !" NormalizerAuto

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

.. _token-delimit:

``TokenDelimit``
^^^^^^^^^^^^^^^^

``TokenDelimit`` extracts token by splitting one or more space
characters (``U+0020``). For example, ``Hello World`` is tokenized to
``Hello`` and ``World``.

``TokenDelimit`` is suitable for tag text. You can extract ``groonga``
and ``full-text-search`` and ``http`` as tags from ``groonga
full-text-search http``.

Here is an example of ``TokenDelimit``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-delimit.log
.. tokenize TokenDelimit "Groonga full-text-search HTTP" NormalizerAuto

``TokenDelimit`` can also specify options.
``TokenDelimit`` has ``delimiter`` option and ``pattern`` option.
``delimiter`` option can split token with a specified characters.

For example, ``Hello,World`` is tokenize to ``Hello`` and ``World``
with ``delimiter`` option as below.

.. groonga-command
.. include:: ../example/reference/tokenizers/token-delimit-delimiter-option.log
.. tokenize 'TokenDelimit("delimiter", ",")' "Hello,World"

``pattern`` option can split token with a regular expression.
You can except needless space by ``pattern`` option.

For example, ``This is a pen. This is an apple`` is tokenize to ``This is a pen`` and
``This is an apple`` with ``pattern`` option as below.

Normally, when ``This is a pen. This is an apple.`` is splitted by ``.``,
needless spaces are included at the beginning of "This is an apple.".

You can except the needless spaces by a ``pattern`` option as below example.

.. groonga-command
.. include:: ../example/reference/tokenizers/token-delimit-pattern-option.log
.. tokenize 'TokenDelimit("pattern", "\\.\\s*")' "This is a pen. This is an apple."

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

.. _token-mecab:

``TokenMecab``
^^^^^^^^^^^^^^

``TokenMecab`` is a tokenizer based on `MeCab
<https://taku910.github.io/mecab/>`_ part-of-speech and
morphological analyzer.

MeCab doesn't depend on Japanese. You can use MeCab for other
languages by creating dictionary for the languages. You can use `NAIST
Japanese Dictionary <http://osdn.jp/projects/naist-jdic/>`_
for Japanese.

You need to install an additional package to using TokenMecab.
For more detail of how to installing an additional package, see `how to install each OS <http://groonga.org/docs/install.html>`_ .

``TokenMecab`` is good for precision rather than recall. You can find
``東京都`` and ``京都`` texts by ``京都`` query with
:ref:`token-bigram` but ``東京都`` isn't expected. You can find only
``京都`` text by ``京都`` query with ``TokenMecab``.

If you want to support neologisms, you need to keep updating your
MeCab dictionary. It needs maintain cost. (:ref:`token-bigram` doesn't
require dictionary maintenance because :ref:`token-bigram` doesn't use
dictionary.) `mecab-ipadic-NEologd : Neologism dictionary for MeCab
<https://github.com/neologd/mecab-ipadic-neologd>`_ may help you.

Here is an example of ``TokenMeCab``. ``東京都`` is tokenized to ``東京``
and ``都``. They don't include ``京都``:

.. groonga-command
.. include:: ../example/reference/tokenizers/token-mecab.log
.. tokenize TokenMecab "東京都"

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
