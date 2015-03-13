.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tokenizers

Tokenizers
==========

Summary
-------

Groonga has tokenizer module that tokenizes text. It is used when
indexing text and searching by query.

.. figure:: /images/reference/tokenizers/used-when-indexing.png
   :align: center
   :width: 80%

   Tokenizer is used when indexing text.

.. figure:: /images/reference/tokenizers/used-when-searching.png
   :align: center
   :width: 80%

   Tokenizer is used when searching by query.


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
