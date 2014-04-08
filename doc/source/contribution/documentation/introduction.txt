.. -*- rst -*-

.. highlightlang:: none

Introduction
============

This documentation describes about how to write, generate and manage
Groonga documentation.

Install depended softwares
--------------------------

Groonga uses Sphinx_ as documentation tool. Groonga requires newer
Sphinx. So Groonga clones the latest Sphinx from Sphinx repository
automatically. You need only Mercurial_, Docutils_ and Jinja_.

.. _Sphinx: http://sphinx.pocoo.org/
.. _Mercurial: http://mercurial.selenic.com/
.. _Docutils: http://docutils.sourceforge.net/
.. _Jinja: http://jinja.pocoo.org/

Here are command lines to install needed softwares.

Debian GNU/Linux, Ubuntu::

  % sudo apt-get install -V -y mercurial python-docutils python-jinja2

CentOS, Fedora::

  % sudo yum install -y mercurial python-docutils python-jinja2

OS X::

  % brew install mercurial
  % pip install docutils
  % pip install jinja

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
