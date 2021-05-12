.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-unigram:

``TokenUnigram``
================

Summary
-------

``TokenUnigram`` is similar to :ref:`token-bigram`. The differences
between them is token unit.

Syntax
------

``TokenUnigram`` hasn't parameter::

  TokenUnigram

Usage
-----

If normalizer is used, ``TokenUnigram`` uses white-space-separate like
tokenize method for ASCII characters. ``TokenUnigram`` uses unigram
tokenize method for non-ASCII characters.

If ``TokenUnigram`` tokenize non-ASCII charactors, ``TokenUnigram`` uses
1 character per token as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-unigram-non-ascii.log
.. tokenize TokenUnigram "日本語の勉強" NormalizerAuto
