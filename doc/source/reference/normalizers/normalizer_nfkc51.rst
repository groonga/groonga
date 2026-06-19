.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-nfkc51:

``NormalizerNFKC51``
=====================

.. deprecated:: 14.1.3

   Use :doc:`./normalizer_nfkc` instead.

   ``NormalizerNFKC51`` and ``NormalizerNFKC("version", "5.0.0")`` are equal.

   It is called ``NormalizerNFKC51``, but it is for Unicode version 5.0.

Summary
-------

``NormalizerNFKC51`` normalizes texts by Unicode NFKC (Normalization
Form Compatibility Composition) for Unicode version 5.0. It supports
only UTF-8 encoding.

.. note::

   It is called ``NormalizerNFKC51``, but it is for Unicode version 5.0.

Normally you don't need to use ``NormalizerNFKC51`` explicitly. You can
use ``NormalizerAuto`` instead.

Syntax
------

``NormalizerNFKC51`` hasn't parameter::

  NormalizerNFKC51

Usage
-----

Here is an example that uses ``NormalizerNFKC51`` normalizer:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc51.log
.. table_create NFKC51Lexicon TABLE_HASH_KEY ShortText --normalizer NormalizerNFKC51

