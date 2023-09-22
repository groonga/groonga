.. -*- rst -*-

Travis CI
=========

This section describes how to use Groonga on `Travis CI
<http://travis-ci.org/>`_. Travis CI is a hosted continuous
integration service for the open source community.

You can use Travis CI for your open source software. This section only
describes about Groonga related configuration. See `Travis CI:
Documentation <http://about.travis-ci.org/docs/>`_ about general
Travis CI configuration.

Configuration
-------------

Travis CI is running on 64-bit Ubuntu 14.04 LTS Server Edition. (See `Travis CI: About
Travis CI Environment
<http://about.travis-ci.org/docs/user/ci-environment/>`_.)  You can
use apt-line for Ubuntu 14.04 LTS provided by Groonga project to install
Groonga on Travis CI.

You can custom build lifecycle by ``.travis.yml``. (See `Travis CI:
Conifugration your Travis CI build with .travis.yml
<http://about.travis-ci.org/docs/user/build-configuration/>`_.) You
can use ``before_install`` hook or ``install`` hook. You should use
``before_install`` if your software uses a language that is supported
by Travis CI such as Ruby. You should use ``install`` otherwise.

Add the following ``sudo`` and ``before_install`` configuration to
``.travis.yml``::

  sudo: required
  before_install:
    - curl --silent --location https://raw.githubusercontent.com/groonga/groonga/HEAD/data/travis/setup.sh | sh

``sudo: required`` configuration is required because ``sudo`` command
is used in the setup script.

If you need to use ``install`` hook instead of ``before_install``, you
just have to replace ``before_install:`` with ``install:``.

With the above configuration, you can use Groonga for your build.

Examples
--------

Here are open source software that use Groonga on Travis CI:

  * `rroonga <http://ranguba.org/#about-rroonga>`_ (Ruby bindings)

    * `rroonga on Travis CI <http://travis-ci.org/#!/ranguba/rroonga>`_
    * `.travis.yml for rroonga <https://github.com/ranguba/rroonga/blob/master/.travis.yml>`_

  * `nroonga <http://nroonga.github.com/>`_ (node.js bindings)

    * `nroonga on Travis CI <http://travis-ci.org/#!/nroonga/nroonga>`_
    * `.travis.yml for nroonga <https://github.com/nroonga/nroonga/blob/master/.travis.yml>`_

  * `logaling-command <http://logaling.github.com/>`_ (A glossary management command line tool)

    * `logaling-command on Travis CI <http://travis-ci.org/#!/logaling/logaling-command>`_
    * `.travis.yml for logaling-command <https://github.com/logaling/logaling-command/blob/master/.travis.yml>`_
