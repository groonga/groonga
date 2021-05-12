.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram:

``TokenBigram``
===============

Summary
-------

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

Syntax
------

``TokenBigram`` hasn't parameter::

  TokenBigram

Usage
-----

``TokenBigram`` behavior is different when it's worked with any
:doc:`/reference/normalizers`.

If no normalizer is used, ``TokenBigram`` uses pure bigram (all tokens
except the last token have two characters) tokenize method:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-no-normalizer.log
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
.. include:: ../../example/reference/tokenizers/token-bigram-ascii-and-white-space-with-normalizer.log
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
.. include:: ../../example/reference/tokenizers/token-bigram-ascii-and-character-type-change-with-normalizer.log
.. tokenize TokenBigram "100cents!!!" NormalizerAuto

Here is an example that ``TokenBigram`` uses bigram tokenize method
for non-ASCII characters.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-non-ascii-with-normalizer.log
.. tokenize TokenBigram "日本語の勉強" NormalizerAuto
