.. -*- rst -*-

Others
======

This section describes how to install Groonga from source on UNIX like
environment.

To get more detail about installing Groonga from source on the
specific environment, find the document for the specific environment
from :doc:`/install`.

Dependencies
------------

Groonga doesn't require any special libraries but requires some tools
for build.

Tools
^^^^^

Here are required tools:

  * ``wget``, ``curl`` or Web browser for downloading source archive
  * ``tar`` and ``gzip`` for extracting source archive
  * shell
    (many shells such as ``dash``, ``bash`` and ``zsh`` will work)
  * C compiler and C++ compiler
    (``gcc`` and ``g++`` are supported but other compilers may work)
  * ``make`` (GNU make is supported but other make like BSD make will work)

You must get them ready.

You can use `CMake <http://www.cmake.org/>`_ instead of shell but this
document doesn't describe about building with CMake.

Here are optional tools:

  * `pkg-config
    <http://www.freedesktop.org/wiki/Software/pkg-config>`_ for
    detecting libraries
  * `sudo <http://www.gratisoft.us/sudo/>`_ for installing built
    Groonga

You must get them ready if you want to use optional libraries.

Libraries
^^^^^^^^^

All libraries are optional. Here are optional libraries:

  * `MeCab <https://taku910.github.io/mecab/>`_ for tokenizing full-text
    search target document by morphological analysis
  * `KyTea <http://www.phontron.com/kytea/>`_ for tokenizing full-text
    search target document by morphological analysis
  * `ZeroMQ <http://www.zeromq.org/>`_ for :doc:`/reference/suggest`
  * `libevent <http://libevent.org/>`_ for :doc:`/reference/suggest`
  * `MessagePack <http://msgpack.org/>`_ for supporting MessagePack
    output and :doc:`/reference/suggest`
  * `libedit <http://www.thrysoee.dk/editline/>`_ for command line
    editing in :doc:`/reference/executables/groonga`
  * `zlib <http://zlib.net/>`_ for compressing column value
  * `LZ4 <https://lz4.github.io/lz4/>`_ for compressing
    column value
  * `Zstandard <https://facebook.github.io/zstd/>`_ for compressing
    column value

If you want to use those all or some libraries, you need to install
them before installing Groonga.

Build from source
-----------------

Groonga uses GNU build system. So the following is the simplest build
steps::

  % wget https://packages.groonga.org/source/groonga/groonga-12.0.1.tar.gz
  % tar xvzf groonga-12.0.1.tar.gz
  % cd groonga-12.0.1
  % ./configure
  % make
  % sudo make install

After the above steps, :doc:`/reference/executables/groonga` is found in
``/usr/local/bin/groonga``.

The default build will work well but you can customize Groonga at
``configure`` step.

The following describes details about each step.

.. _source-configure:

``configure``
^^^^^^^^^^^^^

First, you need to run ``configure``. Here are important ``configure``
options:

``--prefix=PATH``
+++++++++++++++++

Specifies the install base directory. Groonga related files are
installed under ``${PATH}/`` directory.

The default is ``/usr/local``. In this case, :doc:`/reference/executables/groonga` is
installed into ``/usr/local/bin/groonga``.

Here is an example that installs Groonga into ``~/local`` for an user
use instead of system wide use::

  % ./configure --prefix=$HOME/local

``--localstatedir=PATH``
++++++++++++++++++++++++

Specifies the base directory to place modifiable file such as log
file, PID file and database files. For example, log file is placed at
``${PATH}/log/groonga.log``.

The default is ``/usr/local/var``.

Here is an example that system wide ``/var`` is used for modifiable
files::

  % ./configure --localstatedir=/var

``--with-log-path=PATH``
++++++++++++++++++++++++

Specifies the default log file path. You can override the default log
path is :doc:`/reference/executables/groonga` command's ``--log-path``
command line option. So this option is not critical build option. It's
just for convenient.

The default is ``/usr/local/var/log/groonga.log``. The
``/usr/local/var`` part is changed by ``--localstatedir`` option.

Here is an example that log file is placed into shared NFS directory
``/nfs/log/groonga.log``::

  % ./configure --with-log-path=/nfs/log/groonga.log

``--with-default-encoding=ENCODING``
++++++++++++++++++++++++++++++++++++

Specifies the default encoding. Available encodings are ``euc_jp``,
``sjis``, ``utf8``, ``latin1``, ``koi8r`` and ``none``.

The default is ``utf8``.

Here is an example that Shift_JIS is used as the default encoding::

  % ./configure --with-default-encoding=sjis

.. _install-configure-with-match-escalation-threshold:

``--with-match-escalation-threshold=NUMBER``
++++++++++++++++++++++++++++++++++++++++++++

Specifies the default match escalation threshold. See
:ref:`select-match-escalation-threshold` about match
escalation threshold. -1 means that match operation never escalate.

The default is 0.

Here is an example that match escalation isn't used by default::

  % ./configure --with-match-escalation-threshold=-1

``--with-zlib``
+++++++++++++++

Enables column value compression by zlib.

The default is disabled.

Here is an example that enables column value compression by zlib::

  % ./configure --with-zlib

``--with-lz4``
++++++++++++++

Enables column value compression by LZ4.

The default is disabled.

Here is an example that enables column value compression by LZ4::

  % ./configure --with-lz4

``--with-message-pack=MESSAGE_PACK_INSTALL_PREFIX``
+++++++++++++++++++++++++++++++++++++++++++++++++++

Specifies where MessagePack is installed. If MessagePack isn't
installed with ``--prefix=/usr``, you need to specify this option with
path that you use for building MessagePack.

If you installed MessagePack with ``--prefix=$HOME/local`` option, you
should specify ``--with-message-pack=$HOME/local`` to Groonga's
``configure``.

The default is ``/usr``.

Here is an example that uses MessagePack built with
``--prefix=$HOME/local`` option::

  % ./configure --with-message-pack=$HOME/local

``--with-munin-plugins``
++++++++++++++++++++++++

Installs Munin plugins for Groonga. They are installed into
``${PREFIX}/share/groonga/munin/plugins/``.

Those plugins are not installed by default.

Here is an example that installs Munin plugins for Groonga::

  % ./configure --with-munin-plugins

``--with-package-platform=PLATFORM``
++++++++++++++++++++++++++++++++++++

Installs platform specific system management files such as init
script. Available platforms are ``centos``, ``centos5``, ``centos6``,
``centos7`` and ``fedora``. Platform starts with ``centos`` are for
Red Hat and Red Hat clone distributions such as CentOS. If ``centos``
is specified, distribution version is guessed. ``fedora`` is for
Fedora.

Those system management files are not installed by default.

Here is an example that installs CentOS specific system management
files::

  % ./configure --with-package-platform=centos

``--help``
++++++++++

Shows all ``configure`` options.

``make``
^^^^^^^^

``configure`` is succeeded, you can build Groonga by ``make``::

  % make

If you have multi cores CPU, you can make faster by using ``-j``
option. If you have 4 cores CPU, it's good for using ``-j4`` option::

  % make -j4

If you get some errors by ``make``, please report them to us:
:doc:`/contribution/report`

``make install``
^^^^^^^^^^^^^^^^

Now, you can install built Groonga!::

  % sudo make install

If you have write permission for ``${PREFIX}``, you don't need to use
``sudo``. e.g. ``--prefix=$HOME/local`` case. In this case, use ``make
install``::

  % make install
