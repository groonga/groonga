.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-auto:

``NormalizerAuto``
==================

Summary
-------

Normally you should use ``NormalizerAuto``
normalizer. ``NormalizerAuto`` was the normalizer for Groonga 2.0.9 or
earlier. ``KEY_NORMALIZE`` flag in ``table_create`` on Groonga 2.0.9
or earlier equals to ``--normalizer NormalizerAuto`` option in
``table_create`` on Groonga 2.1.0 or later.

``NormalizerAuto`` supports all encoding. It uses Unicode NFKC
(Normalization Form Compatibility Composition) for UTF-8 encoding
text. It uses encoding specific original normalization for other
encodings. The results of those original normalization are similar to
NFKC.

Syntax
------

``NormalizerAuto`` hasn't parameter::

  NormalizerAuto

Usage
-----

``NormalizerAuto`` normalizes half-width katakana (such as U+FF76 HALFWIDTH KATAKANA
LETTER KA) + half-width katakana voiced sound mark (U+FF9E HALFWIDTH
KATAKANA VOICED SOUND MARK) to full-width katakana with
voiced sound mark (U+30AC KATAKANA LETTER GA). The former is two
characters but the latter is one character.

Here is an example that uses ``NormalizerAuto`` normalizer:

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-auto.log
.. table_create NormalLexicon TABLE_HASH_KEY ShortText --normalizer NormalizerAuto

See also
----------

* :doc:`../commands/normalize`
