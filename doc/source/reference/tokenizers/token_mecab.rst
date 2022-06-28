.. -*- rst -*-

.. groonga-command
.. database: tokenizers

``TokenMecab``
================

Summary
-------

``TokenMecab`` is a tokenizer based on `MeCab
<https://taku910.github.io/mecab/>`_ part-of-speech and
morphological analyzer.

MeCab doesn't depend on Japanese. You can use MeCab for other
languages by creating dictionary for the languages. You can use `NAIST
Japanese Dictionary <http://osdn.jp/projects/naist-jdic/>`_
for Japanese.

You need to install an additional package to using TokenMecab.
For more detail of how to installing an additional package, see :doc:`/install` .

``TokenMecab`` is good for precision rather than recall. You can find
``東京都`` and ``京都`` texts by ``京都`` query with
:ref:`token-bigram` but ``東京都`` isn't expected. You can find only
``京都`` text by ``京都`` query with ``TokenMecab``.

If you want to support neologisms, you need to keep updating your
MeCab dictionary. It needs maintain cost. (:ref:`token-bigram` doesn't
require dictionary maintenance because :ref:`token-bigram` doesn't use
dictionary.) `mecab-ipadic-NEologd : Neologism dictionary for MeCab
<https://github.com/neologd/mecab-ipadic-neologd>`_ may help you.

Syntax
------

``TokenMecab`` has optional parameter.

No options::

  TokenMecab

Specify option::

  TokenMecab("include_class", true)

  TokenMecab("target_class", "a_part_of_speech")

  TokenMecab("include_reading", true)

  TokenMecab("include_form", true)

  TokenMecab("use_reading", true)

Specify multiple options::

  TokenMecab("target_class", "名詞", "include_reading", true)

``TokenMecab`` also specify multiple options as above.
You can also specify mingle multiple options except above example.

Usage
-----

Simple usage
------------

Here is an example of ``TokenMeCab``. ``東京都`` is tokenized to ``東京``
and ``都``. They don't include ``京都``:

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab.log
.. tokenize TokenMecab "東京都"

``TokenMecab`` can also specify options.
``TokenMecab`` has ``target_class`` option, ``include_class`` option,
``include_reading`` option, ``include_form`` option and ``use_reading`` option.

``target_class`` option searches a token of specifying a part-of-speech.
For example, you can search only a noun as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-target-class-option.log
.. tokenize 'TokenMecab("target_class", "名詞")' '彼の名前は山田さんのはずです。'

``include_class`` option outputs class and subclass in MeCab's metadata as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-include-class-option.log
.. tokenize 'TokenMecab("include_class", true)' '彼の名前は山田さんのはずです。'

You can exclude needless token with ``target_class`` and class and sub class of this option outputs.

``include_reading`` outputs reading in MeCab's metadata as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-include-reading-option.log
.. tokenize 'TokenMecab("include_reading", true)' '彼の名前は山田さんのはずです。'

You can get reading of a token with this option.

``include_form`` outputs inflected_type, inflected_form and base_form in MeCab's metadata as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-include-form-option.log
.. tokenize 'TokenMecab("include_form", true)' '彼の名前は山田さんのはずです。'

``use_reading`` supports a search by kana.
This option is useful for countermeasure of orthographical variants because it searches with kana.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-use-reading-option.log
.. tokenize 'TokenMecab("include_form", true)' '彼の名前は山田さんのはずです。'

Advanced usage
--------------

``target_class`` option can also specify subclasses and exclude or add specific
part-of-speech of specific using + or -.
So, you can also search a noun with excluding non-independent word and suffix of
person name as below.

In this way you can search exclude the noise of token.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-target-class-option-complex.log
.. tokenize 'TokenMecab("target_class", "-名詞/非自立", "target_class", "-名詞/接尾/人名", "target_class", "名詞")' '彼の名前は山田さんのはずです。'

In addition, you can get reading of a token that exclude the noise with ``include_reading`` option as below.

.. groonga-command
.. include:: ../../example/reference/tokenizers/token-mecab-target-class-and-include-class-option.log
.. tokenize 'TokenMecab("target_class", "-名詞/非自立", "target_class", "-名詞/接尾/人名", "target_class", "名詞", "include_reading", true)' '彼の名前は山田さんのはずです。'

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are four optional parameters ``include_class`` , ``target_class`` , ``include_form`` and ``use_reading`` .

``include_class``
"""""""""""""""""

Outputs class and subclass in MeCab's metadata.

``target_class``
""""""""""""""""

Outputs a token of specifying a part-of-speech.

``include_reading``
"""""""""""""""""""

Outputs reading in MeCab's metadata.

``include_form``
""""""""""""""""

Outputs inflected_type, inflected_form and base_form in MeCab's metadata.

``use_reading``
"""""""""""""""

Outputs reading of token.

See also
----------

* :doc:`../commands/tokenize`
