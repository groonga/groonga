.. -*- rst -*-

macOS
=====

This section describes how to install Groonga on macOS. You can
install Groonga by `MacPorts <http://www.macports.org/>`__ or
`Homebrew <http://mxcl.github.com/homebrew/>`__.

.. _macports:

MacPorts
--------

Install::

  % sudo port install groonga

.. _homebrew:

Homebrew
--------

Install::

  % brew install groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, specify ``--with-mecab`` option::

  % brew install groonga --with-mecab

Then install and configure MeCab dictionary.

Install::

  % brew install mecab-ipadic

Build from source
-----------------

Build from source is for developers.

See :doc:`/install/cmake` .
