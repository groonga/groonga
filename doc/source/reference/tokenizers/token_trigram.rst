.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-trigram:

``TokenTrigram``
================

Summary
-------

``TokenTrigram`` is similar to :ref:`token-bigram`. The differences
between them is token unit.

Syntax
------

``TokenTrigram`` hasn't parameter::

  TokenTrigram

Usage
-----

If normalizer is used, ``TokenTrigram`` uses white-space-separate like
tokenize method for ASCII characters. ``TokenTrigram`` uses trigram
tokenize method for non-ASCII characters.

If ``TokenTrigram`` tokenize non-ASCII charactors, ``TokenTrigram`` uses
3 character per token as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-trigram-non-ascii.log
.. tokenize TokenTrigram "日本語の勉強" NormalizerAuto
