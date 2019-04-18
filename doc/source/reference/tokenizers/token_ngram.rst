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

Simple usage
------------

Here is an example of ``TokenNgram``.

If we use ``TokenNgram`` in the nothing of option.
``TokenNgram`` behavior is the same as ``TokenBigram`` as below:

* If no normalizer is used.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-no-option.log
.. tokenize --tokenizer TokenNgram --string "Hello World"

* If normalizer is used.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-no-option-with-normalizer.log
.. tokenize --tokenizer TokenNgram --string "Hello World" --normalizer NormalizerAuto


Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are eight optional parameters.

.. _token-ngram-n:

``n``
"""""

This option shows ``N`` of Ngram. For example, ``n`` is ``3`` for trigram.

.. _token-ngram-loose-symbol:

``loose_symbol``
""""""""""""""""

Tokenize keywords including symbols, to be searched by both queries with/without symbols.
For example, a keyword ``090-1111-2222`` will be found by any of ``09011112222``, ``090``, ``1111``, ``2222`` and ``090-1111-2222``.

.. _token-ngram-loose-blank:

``loose_blank``
"""""""""""""""

Tokenize keywords including blanks, to be searched by both queries with/without blanks.
For example, a keyword ``090 1111 2222`` will be found by any of ``09011112222``, ``090``, ``1111``, ``2222`` and ``090 1111 2222``.

.. _token-ngram-remove-blank:

``remove_blank``
""""""""""""""""

Tokenize keywords including blanks, to be searched by queries without blanks.
For example, a keyword ``090 1111 2222`` will be found by any of ``09011112222``, ``090``, ``1111`` or ``2222``.

Note that the keyword won’t be found by a query including blanks like ``090 1111 2222``.

.. _token-ngram-report-source-location:

``report_source_location``
""""""""""""""""""""""""""

This option tells us a location of token of highlight target when we highlight token by ``highlight_html``.

We only use this option when we want to highlight token by ``highlight_html``.

When Groonga tokenizes token, always used ``NormalizerAuto`` to normalizer until now.
Therefore, if we use ``NormalizerNFKC100`` to normalizer, sometimes the position of the highlight shift.
Because we use different normalizer to normalize token.

This option is its to reduce the shift of the position of the highlight.

.. _token-ngram-unify-alphabet:

``unify_alphabet``
""""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for ASCII character.

.. _token-ngram-unify-symbol:

``unify_symbol``
""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for symbols.
``TokenNgram("unify_symbol", false)`` is same behavior of ``TokenBigramSplitSymbol``.

.. _token-ngram-unify-digit:

``unify_digit``
"""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for digits.

See also
----------

* :doc:`../commands/tokenize`
