.. -*- rst -*-

.. highlightlang:: none

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

:ref:`token-bigram` uses 2 characters per
token. ``TokenTrigram`` uses 3 characters per token as below example.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-trigram.log
.. tokenize TokenTrigram "10000cents!!!!!" NormalizerAuto
