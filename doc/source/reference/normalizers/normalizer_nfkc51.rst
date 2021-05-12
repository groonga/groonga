.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-nfkc51:

``NormalizerNFKC51``
=====================

Summary
-------

``NormalizerNFKC51`` normalizes texts by Unicode NFKC (Normalization
Form Compatibility Composition) for Unicode version 5.1. It supports
only UTF-8 encoding.

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

