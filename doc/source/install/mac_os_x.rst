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

Configure::

  % sed -i '' -e 's,dicrc.*=.*,dicrc = /usr/local/lib/mecab/dic/ipadic,g' /usr/local/etc/mecabrc

Build from source
-----------------

Install `Xcode <https://developer.apple.com/xcode/>`_.

Download source::

  % curl -O https://packages.groonga.org/source/groonga/groonga-12.0.6.tar.gz
  % tar xvzf groonga-12.0.6.tar.gz
  % cd groonga-12.0.6

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(/usr/sbin/sysctl -n hw.ncpu)

Install::

  % sudo make install
