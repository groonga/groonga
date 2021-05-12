.. -*- rst -*-

Introduction
============

This documentation describes about how to write, generate and manage
Groonga documentation.

Install depended software
-------------------------

Groonga uses Sphinx_ as documentation tool.

.. _Sphinx: http://sphinx.pocoo.org/

Here are command lines to install Sphinx.

Debian GNU/Linux, Ubuntu::

  % sudo apt-get install -V -y python-sphinx

CentOS, Fedora::

  % sudo yum install -y python-pip
  % sudo pip install sphinx

OS X::

  % brew install python
  % brew install gettext
  % export PATH=`brew --prefix gettext`/bin:$PATH
  % pip install sphinx

If the version of Python on your platform is too old, you'll need to
install a newer version of Python 2.7 by your hand. For example, here
are installation steps based on `pyenv
<https://github.com/yyuu/pyenv>`_::

  % pyenv install 2.7.11
  % pyenv global 2.7.11
  % pip install sphinx

Run ``configure`` with ``--enable-document``
--------------------------------------------

Groonga disables documentation generation by default. You need to
enable it explicitly by adding ``--enable-document`` option to
``configure``::

  % ./configure --enable-document

Now, your Groonga build is documentation ready.

Generate HTML
-------------

You can generate HTML by the following command::

  % make -C doc html

You can find generated HTML documentation at ``doc/locale/en/html/``.

Update
------

You can find sources of documentation at ``doc/source/``. The sources
should be written in English. See :doc:`i18n` about how to translate
documentation.

You can update the target file when you update the existing
documentation file.

You need to update file list after you add a new file, change file
path and delete existing file. You can update file list by the
following command::

  % make -C doc update-files

The command updates ``doc/files.am``.
