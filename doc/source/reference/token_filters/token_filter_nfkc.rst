.. -*- rst -*-

.. groonga-command
.. database: token_filters_nfkc

``TokenFilterNFKC``
===================

Summary
-------

.. versionadded:: 14.1.3

This token filter can use the same option by :ref:`normalizer-nfkc`.
This token filter is used to normalize after tokenizing.
Because, if you normalize before tokenizing with ``TokenMecab`` , the meaning of a token may be lost.

Syntax
------

``TokenFilterNFKC`` has optional parameter.

No options::

  TokenFilterNFKC

``TokenFilterNFKC`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition).

Example of option specification::

  TokenFilterNFKC("version", "16.0.0")

  TokenFilterNFKC("unify_kana", true)

  TokenFilterNFKC("unify_hyphen", true)

  TokenFilterNFKC("unify_to_romaji", true)

Other options available same as :ref:`normalizer-nfkc`.

Usage
-----

Simple usage
^^^^^^^^^^^^

Normalization is the same as in :ref:`normalizer-nfkc`, so here are a few examples of how to use the options.

Here is an example of ``TokenFilterNFKC``. ``TokenFilterNFKC`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition).

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc.rst
.. tokenize TokenDelimit "©" --token_filters TokenFilterNFKC

Here is an example of :ref:`normalizer-nfkc-version` option.
You can specify the Unicode version for this option.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc-version.rst
.. tokenize TokenDelimit "©" --token_filters 'TokenFilterNFKC("version", "16.0.0")'

Here is an example of :ref:`normalizer-nfkc-unify-kana` option.

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc-unify-kana.rst
.. tokenize TokenDelimit "あイｳｪおヽヾ" --token_filters 'TokenFilterNFKC("unify_kana", true)'


Here is an example of :ref:`normalizer-nfkc-unify-hyphen` option.
This option enables normalize hyphen to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc-unify-hyphen.rst
.. tokenize TokenDelimit "-˗֊‐‑‒–⁃⁻₋−" --token_filters 'TokenFilterNFKC("unify_hyphen", true)'

Here is an example of :ref:`normalizer-nfkc-unify-to-romaji` option.
This option enables normalize hiragana and katakana to romaji as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc-unify-to-romaji.rst
.. tokenize TokenDelimit "アァイィウゥエェオォ" --token_filters  'TokenFilterNFKC("unify_to_romaji", true)'

Advanced usage
^^^^^^^^^^^^^^

You can output all input string as hiragana with cimbining ``TokenFilterNFKC`` with ``use_reading`` option of ``TokenMecab`` as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc-with-token-mecab.rst
.. tokenize   'TokenMecab("use_reading", true)'   "私は林檎を食べます。"   --token_filters 'TokenFilterNFKC("unify_kana", true)'

Parameters
----------

See :ref:`normalizer-nfkc-parameters` in ``NormalizerNFKC`` for details.
