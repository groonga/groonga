.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-ngram:

``TokenNgram``
==============

Summary
-------

``TokenNgram`` can change define its behavior dynamically via its options.
For example, we can use it as unigram, bigram, trigram on with changing ``n`` option value as below.

Uni-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-uni-gram.log
.. tokenize --tokenizer 'TokenNgram("n", 1)' --string "Hello World"

Bi-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-bi-gram.log
.. tokenize --tokenizer 'TokenNgram("n", 2)' --string "Hello World"

Tri-gram:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-tri-gram.log
.. tokenize --tokenizer 'TokenNgram("n", 3)' --string "Hello World"

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

We can also specify multiple options at the same time except the above examples.

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

Advanced usage
--------------

We can specify multiple options for ``TokenNgram``.

For example, we can deal with variantsas as phone number by using ``loose_symbol`` and ``loose_blank`` as below.

We can search ``0123(45)6789``, ``0123-45-6789`` and, ``0123 45 6789`` by using ``0123456789`` as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-loose-symbol-and-loose-blank.log
.. tokenize --tokenizer 'TokenNgram("loose_symbol", true, "loose_blank", true)' --string "0123(45)6789" --normalizer NormalizerAuto
.. tokenize --tokenizer 'TokenNgram("loose_symbol", true, "loose_blank", true)' --string "0123-45-6789" --normalizer NormalizerAuto
.. tokenize --tokenizer 'TokenNgram("loose_symbol", true, "loose_blank", true)' --string "0123 45 6789" --normalizer NormalizerAuto

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

When Groonga tokenize texts that target highlight, always used ``NormalizerAuto`` to normalizer until now.
Therefore, if we use ``NormalizerNFKC100`` to normalizer, sometimes it
can't find the position of the highlight as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-no-report-source-location.log
.. table_create Entries TABLE_NO_KEY
.. column_create Entries body COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText --normalizer 'NormalizerNFKC100'
.. column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body
.. load --table Entries
.. [
.. {"body": "ア㌕Ａz"}
.. ]
.. select Entries   --match_columns body   --query 'グラム'   --output_columns 'highlight_html(body, Terms)'

Because we use different normalizer to normalize token.

This option is used to reduce the shift of the position of the highlight as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-report-source-location.log
.. table_create Entries TABLE_NO_KEY
.. column_create Entries body COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText   --default_tokenizer 'TokenNgram("report_source_location", true)'   --normalizer 'NormalizerNFKC100'
.. column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body
.. load --table Entries
.. [
.. {"body": "ア㌕Ａz"}
.. ]
.. select Entries   --match_columns body   --query 'グラム'   --output_columns 'highlight_html(body, Terms)'

.. _token-ngram-unify-alphabet:

``unify_alphabet``
""""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for ASCII character.

Default value of this option is true.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-unify-alphabet.log
.. tokenize --tokenizer 'TokenNgram("unify_alphabet", false)' --string "Hello World" --normalizer NormalizerAuto

.. _token-ngram-unify-symbol:

``unify_symbol``
""""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for symbols.
``TokenNgram("unify_symbol", false)`` is same behavior of ``TokenBigramSplitSymbol``.

Default value of this option is true.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-unify-symbol.log
.. tokenize --tokenizer 'TokenNgram("unify_symbol", false)' --string "___---" --normalizer NormalizerAuto

.. _token-ngram-unify-digit:

``unify_digit``
"""""""""""""""

If we set false, ``TokenNgram`` uses bigram tokenize method for digits.

Default value of this option is true.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-ngram-unify-digit.log
.. tokenize --tokenizer 'TokenNgram("unify_digit", false)' --string "012345 6789" --normalizer NormalizerAuto

See also
----------

* :doc:`../commands/tokenize`
