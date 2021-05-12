.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-bigram-ignore-blank:

``TokenBigramIgnoreBlank``
==========================

Summary
-------

``TokenBigramIgnoreBlank`` is similar to :ref:`token-bigram`. The
difference between them is blank handling. ``TokenBigramIgnoreBlank``
ignores white-spaces in continuous symbols and non-ASCII characters.

Syntax
------

``TokenBigramIgnoreBlank`` hasn't parameter::

  TokenBigramIgnoreBlank

Usage
-----

You can find difference of them by ``日 本 語 ! ! !`` text because it
has symbols and non-ASCII characters.

Here is a result by :ref:`token-bigram` :

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-with-white-spaces.log
.. tokenize TokenBigram "日 本 語 ! ! !" NormalizerAuto

Here is a result by ``TokenBigramIgnoreBlank``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-bigram-ignore-blank-with-white-spaces.log
.. tokenize TokenBigramIgnoreBlank "日 本 語 ! ! !" NormalizerAuto
