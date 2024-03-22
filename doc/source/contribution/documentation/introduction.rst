.. -*- rst -*-

Introduction
============

This documentation describes about how to write, generate and manage
Groonga documentation.

How to fork and clone Groonga repository
----------------------------------------

Contributing to Groonga's documentation begins with forking and cloning the Groonga repository.
These actions are essential first steps that enable personal modifications and experimentation in your personal repository.
And also, it enables you to submit them as your contributions to the Groonga repository. Follow these steps.

1. Go to https://github.com/groonga/groonga on GitHub
2. Click the ``Fork`` button to create a copy of the repository
3. Clone your Groonga repository with the following command

.. code-block:: console

   % git clone --recursive git@github.com:${YOUR_GITHUB_ACCOUNT}/groonga.git

Install depended software
-------------------------

Groonga uses Sphinx_ as documentation tool.

.. _Sphinx: https://www.sphinx-doc.org/

Here are command lines to install Sphinx.

Debian GNU/Linux, Ubuntu:

.. code-block:: console

   % ./setup.sh
   % sudo pip install -r doc/requirements.txt
   % (cd doc && bundle install)

AlmaLinux, Fedora:

.. code-block:: console

   % sudo dnf install -y python-pip gettext
   % sudo pip install -r doc/requirements.txt
   % (cd doc && bundle install)

macOS:

.. code-block:: console

   % brew bundle
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

How to edit documentation
-------------------------

The Groonga documentation is written in `reStructuredText (.rst)`_ or `Markdown (.md)`_ . These files are located in the ``doc/source``.
Each page of the documentation corresponds to a ``.rst`` file or a ``.md`` file. By modifying the corresponding file, you can edit the target document.
For Example, if you want to edit this :doc:`introduction` page, you should edit the ``doc/source/contribution/documentation/introduction.rst`` file.
Please find the file you wish to edit and make your changes.

.. _reStructuredText (.rst): https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html
.. _Markdown (.md): https://myst-parser.readthedocs.io/en/latest/

Update
------

You can find sources of documentation at ``doc/source/``. The sources
should be written in English. See :doc:`i18n` about how to translate
documentation.

You can update the target file when you update the existing
documentation file.
