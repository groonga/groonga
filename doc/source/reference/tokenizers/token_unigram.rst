.. -*- rst -*-

.. highlightlang:: none

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

:ref:`token-bigram` uses 2 characters per
token. ``TokenUnigram`` uses 1 character per token as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-unigram.log
.. tokenize TokenUnigram "100cents!!!" NormalizerAuto
