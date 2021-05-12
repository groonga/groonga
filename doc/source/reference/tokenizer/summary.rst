.. -*- rst -*-

Summary
=======

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
.. include:: ../../example/reference/tokenizers/tokenize-example.log
.. tokenize TokenBigram "Hello World"

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
