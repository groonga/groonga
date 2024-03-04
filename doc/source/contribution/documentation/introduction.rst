.. -*- rst -*-

Introduction
============

This documentation describes about how to write, generate and manage
Groonga documentation.

Install depended software
-------------------------

Groonga uses Sphinx_ as documentation tool.

.. _Sphinx: https://www.sphinx-doc.org/

Here are command lines to install Sphinx.

Debian GNU/Linux, Ubuntu:

.. code-block:: console

   % sudo apt-get install -V -y python3-pip gettext
   % sudo pip install -r doc/requirements.txt
   % (cd doc && bundle install)

AlmaLinux, Fedora:

.. code-block:: console

   % sudo dnf install -y python-pip gettext
   % sudo pip install -r doc/requirements.txt
   % (cd doc && bundle install)

macOS:

.. code-block:: console

   % brew install python gettext
   % export PATH=$(brew --prefix gettext)/bin:$PATH
   % pip install -r doc/requirements.txt
   % (cd doc && bundle install)

Run ``cmake`` with ``--preset=doc``
-----------------------------------

Groonga disables documentation generation by default. You need to
enable it explicitly by adding ``--preset=doc`` option to
``cmake``:

.. code-block:: console

   % cmake -S . -B ../groonga.doc --preset=doc

Now, your Groonga build is documentation ready.

Generate HTML
-------------

You can generate HTML by the following command:

.. code-block:: console

   % cmake --build ../groonga.doc

You can find generated HTML documentation at ``../groonga.doc/doc/en/html/``.

Update
------

You can find sources of documentation at ``doc/source/``. The sources
should be written in English. See :doc:`i18n` about how to translate
documentation.

You can update the target file when you update the existing
documentation file.
