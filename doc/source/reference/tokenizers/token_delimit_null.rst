.. -*- rst -*-

.. groonga-command
.. database: tokenizers

.. _token-delimit-null:

``TokenDelimitNull``
====================

Summary
-------

``TokenDelimitNull`` is similar to :ref:`token-delimit`. The
difference between them is separator character. :ref:`token-delimit`
uses space character (``U+0020``) but ``TokenDelimitNull`` uses NUL
character (``U+0000``).

Syntax
------

``TokenDelimitNull`` hasn't parameter::

  TokenDelimitNull

Usage
-----


``TokenDelimitNull`` is also suitable for tag text.

Here is an example of ``TokenDelimitNull``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-delimit-null.log
.. tokenize TokenDelimitNull "Groonga\u0000full-text-search\u0000HTTP" NormalizerAuto


