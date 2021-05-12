.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-delimit:

``TokenDelimit``
================

Summary
-------

``TokenDelimit`` extracts token by splitting one or more space
characters (``U+0020``). For example, ``Hello World`` is tokenized to
``Hello`` and ``World``.

``TokenDelimit`` is suitable for tag text. You can extract ``groonga``
and ``full-text-search`` and ``http`` as tags from ``groonga
full-text-search http``.

Syntax
------

``TokenDelimit`` has optional parameter.

No options(Extracts token by splitting one or more space characters (``U+0020``))::

  TokenDelimit

Specify delimiter::

  TokenDelimit("delimiter",  "delimiter1", delimiter", "delimiter2", ...)

Specify delimiter with regular expression::

  TokenDelimit("pattern", pattern)

The ``delimiter`` option and a ``pattern`` option are not use at the same time.

Usage
-----

Simple usage
------------

Here is an example of ``TokenDelimit``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit.log
.. tokenize TokenDelimit "Groonga full-text-search HTTP" NormalizerAuto

``TokenDelimit`` can also specify options.
``TokenDelimit`` has ``delimiter`` option and ``pattern`` option.

``delimiter`` option can split token with a specified character.

For example, ``Hello,World`` is tokenized to ``Hello`` and ``World``
with ``delimiter`` option as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit-delimiter-option.log
.. tokenize 'TokenDelimit("delimiter", ",")' "Hello,World"

``pattern`` option can split token with a regular expression.
You can except needless space by ``pattern`` option.

For example, ``This is a pen. This is an apple`` is tokenized to ``This is a pen`` and
``This is an apple`` with ``pattern`` option as below.

Normally, when ``This is a pen. This is an apple.`` is splitted by ``.``,
needless spaces are included at the beginning of "This is an apple.".

You can except the needless spaces by a ``pattern`` option as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit-pattern-option.log
.. tokenize 'TokenDelimit("pattern", "\\.\\s*")' "This is a pen. This is an apple."

Advanced usage
--------------

``delimiter`` option can also specify multiple delimiters.

For example, ``Hello, World`` is tokenized to ``Hello`` and ``World``.
``,`` and `` `` are delimiters in below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit-delimiter-option-multiple-delimiters.log
.. tokenize 'TokenDelimit("delimiter", ",", "delimiter", " ")' "Hello, World"

You can extract token in complex conditions by ``pattern`` option.

For example, ``これはペンですか！？リンゴですか？「リンゴです。」`` is tokenize to ``これはペンですか`` and ``リンゴですか``, ``「リンゴです。」`` with ``delimiter`` option as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit-pattern-option-with-complex-pattern.log
.. tokenize 'TokenDelimit("pattern", "([。！？]+(?![）」])|[\\r\\n]+)\\s*")' "これはペンですか！？リンゴですか？「リンゴです。」"

``\\s*`` of the end of above regular expression match 0 or more spaces after a delimiter.

``[。！？]+`` matches 1 or more ``。`` or ``！``, ``？``.
For example, ``[。！？]+`` matches ``！？`` of ``これはペンですか！？``.

``(?![）」])`` is negative lookahead.
``(?![）」])`` matches if a character is not matched ``）`` or ``」``.
negative lookahead interprets in combination regular expression of just before.

Therefore it interprets ``[。！？]+(?![）」])``.

``[。！？]+(?![）」])`` matches if there are not ``）`` or ``」`` after ``。`` or ``！``, ``？``.

In other words, ``[。！？]+(?![）」])`` matches ``。`` of ``これはペンですか。``. But ``[。！？]+(?![）」])`` doesn't match ``。`` of ``「リンゴです。」``.
Because there is ``」`` after ``。``.

``[\\r\\n]+`` match 1 or more newline character.

In conclusion, ``([。！？]+(?![）」])|[\\r\\n]+)\\s*`` uses ``。`` and ``！`` and ``？``, newline character as delimiter. However, ``。`` and ``!``, ``？`` are not delimiters if there is ``）`` or ``」`` after ``。`` or ``！``, ``？``.

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are two optional parameters ``delimiter`` and ``pattern``.

``delimiter``
"""""""""""""

Split token with a specified one or more characters.

You can use one or more characters for a delimiter.

``pattern``
"""""""""""

Split token with a regular expression.

See also
----------

* :doc:`../commands/tokenize`
