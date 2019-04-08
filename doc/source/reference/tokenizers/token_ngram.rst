.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tokenizers

.. _token-ngram:

``TokenNgram``
===============

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


``loose_symbol``
""""""""""""""""


``loose_blank``
"""""""""""""""


``remove_blank``
""""""""""""""""


``report_source_location``
""""""""""""""""""""""""""


``unify_alphabet``
""""""""""""""""""


``unify_symbol``
""""""""""""""""


``unify_digit``
"""""""""""""""


See also
----------

* :doc:`../commands/tokenize`
