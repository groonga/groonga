.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tokenizers

.. _token-ngram:

``TokenNgram``
==============

Summary
-------

``TokenNgram`` can define its behavior dynamically via its options.
For example, we can use unigram, bigram, trigram and so on with changing ``n`` option value as below.

Uni-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-uni-gram.log
.. tokenize 'TokenNgram("n", 1)' "Hello World"

Bi-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-bi-gram.log
.. tokenize 'TokenNgram("n", 2)' "Hello World"

tri-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-tri-gram.log
.. tokenize 'TokenNgram("n", 3)' "Hello World"

This tokenizer also has options other than the above.

Syntax
------

``TokenNgram`` has optional parameter.

No options::

  TokenNgram

If we don't use options, ``TokenNgram`` is the same behavior for ``TokenBigram``.

Specify option::

  TokenNgram("n", true)

  TokenNgram("loose_symbol", true)

  TokenNgram("loose_blank", true)

  TokenNgram("remove_blank", true)

  TokenNgram("report_source_location", true)

  TokenNgram("unify_alphabet", true)

  TokenNgram("unify_symbol", true)

  TokenNgram("unify_digit", true)

Specify multiple options::

  TokenNgram("loose_symbol", true, "loose_blank", true)

``TokenNgram`` also specify multiple options as above.
We can also specify mingle multiple options except the above example.

Usage
-----


Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are eight optional parameters.

``n``
"""""

This option shows ``N`` of Ngram. For example, ``n`` is ``3`` for trigram.

``loose_symbol``
""""""""""""""""

Tokenize keywords including symbols, to be searched by both queries with/without symbols.
For example, a keyword ``090-1111-2222`` will be found by any of ``09011112222``, ``090``, ``1111``, ``2222`` and ``090-1111-2222``.

``loose_blank``
"""""""""""""""

Tokenize keywords including blanks, to be searched by both queries with/without blanks.
For example, a keyword ``090 1111 2222`` will be found by any of ``09011112222``, ``090``, ``1111``, ``2222`` and ``090 1111 2222``.

``remove_blank``
""""""""""""""""

Tokenize keywords including blanks, to be searched by queries without blanks.
For example, a keyword ``090 1111 2222`` will be found by any of ``09011112222``, ``090``, ``1111`` or ``2222``.

Note that the keyword won’t be found by a query including blanks like ``090 1111 2222``.

``report_source_location``
""""""""""""""""""""""""""


``unify_alphabet``
""""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for ASCII character.

``unify_symbol``
""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for symbols.
``TokenNgram("unify_symbol", false)`` is same behavior of ``TokenBigramSplitSymbol``.

``unify_digit``
"""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for digits.

See also
----------

* :doc:`../commands/tokenize`
