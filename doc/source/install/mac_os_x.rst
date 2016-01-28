.. -*- rst -*-

.. highlightlang:: none

Mac OS X
========

This section describes how to install Groonga on Mac OS X. You can
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

If you want to use `MeCab <http://mecab.sourceforge.net/>`_ as a
tokenizer, specify ``--with-mecab`` option::

  % brew install groonga --with-mecab

Build from source
-----------------

Install `Xcode <https://developer.apple.com/xcode/>`_.

Download source::

  % curl -O http://packages.groonga.org/source/groonga/groonga-5.1.2.tar.gz
  % tar xvzf groonga-5.1.2.tar.gz
  % cd groonga-5.1.2

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(/usr/sbin/sysctl -n hw.ncpu)

Install::

  % sudo make install
