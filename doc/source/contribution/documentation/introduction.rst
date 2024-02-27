.. -*- rst -*-

Introduction
============

This documentation describes about how to write, generate and manage
Groonga documentation.

Checkout Groonga from the repository
------------------------------------

You should build Groonga documentation from source code at the repository.
Because source code in the repository is the latest.

The Groonga repository is hosted on `GitHub
<https://github.com/groonga/groonga>`_. Checkout the latest source
code from the repository::

  % git clone --recursive git@github.com:groonga/groonga.git
  % cd groonga

Install depended software
-------------------------

Groonga uses Sphinx_ as documentation tool.

.. _Sphinx: http://sphinx.pocoo.org/

Here are command lines to install Sphinx.

Debian GNU/Linux, Ubuntu::

  % sudo apt-get install -V -y python3-pip gettext
  % sudo pip install -r doc/requirements.txt
  % (cd doc && bundle install)

CentOS, Fedora::

  % sudo yum install -y python-pip gettext
  % sudo pip install -r doc/requirements.txt
  % (cd doc && bundle install)

OS X::

  % brew install python
  % brew install gettext
  % export PATH=`brew --prefix gettext`/bin:$PATH
  % pip install -r doc/requirements.txt
  % (cd doc && bundle install)

Run ``cmake`` with ``--preset=doc``
-----------------------------------

Groonga disables documentation generation by default. You need to
enable it explicitly by adding ``--preset=doc`` option to
``cmake``::

  % cmake -S . -B ../groonga.doc --preset=doc

Now, your Groonga build is documentation ready.

Generate HTML
-------------

You can generate HTML by the following command::

  % cmake --build ../groonga.doc

You can find generated HTML documentation at ``../groonga.doc/doc/en/html/``.

Update
------

You can find sources of documentation at ``doc/source/``. The sources
should be written in English. See :doc:`i18n` about how to translate
documentation.

You can update the target file when you update the existing
documentation file.
