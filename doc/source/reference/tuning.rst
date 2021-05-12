.. -*- rst -*-

Tuning
======

Summary
-------

There are some tuning parameters for handling a large database.

Parameters
----------

This section describes tuning parameters.

.. _tuning-max-n-open-files:

The max number of open files per process
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This parameter is for handling a large database.

Groonga creates one or more files per table and column. If your
database has many tables and columns, Groonga process needs to open
many files.

System limits the max number of open files per process. So you need to
relax the limitation.

Here is an expression that compute how many files are opened by
Groonga::

  3 (for DB) +
    N tables +
    N columns (except index clumns) +
    (N index columns * 2) +
    X (the number of plugins etc.)

Here is an example schema::

  table_create Entries TABLE_HASH_KEY ShortText
  column_create Entries content COLUMN_SCALAR Text
  column_create Entries n_likes COLUMN_SCALAR UInt32
  table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
  column_create Terms entries_key_index COLUMN_INDEX|WITH_POSITION Entries _key
  column_create Terms entries_content_index COLUMN_INDEX|WITH_POSITION Entries content

This example opens at least 11 files::

  3 +
    2 (Entries and Terms) +
    2 (Entries.content and Entries.n_likes) +
    4 (Terms.entries_key_index and Terms.entries_content_index) +
    X = 11 + X

.. _tuning-memory-usage:

Memory usage
^^^^^^^^^^^^

This parameter is for handling a large database.

Groonga maps database files onto memory and accesses to them. Groonga
doesn't maps unnecessary files onto memory. Groonga maps files when
they are needed.

If you access to all data in database, all database files are mapped
onto memory. If total size of your database files is 6GiB, your
Groonga process uses 6GiB memory.

Normally, your all database files aren't mapped onto memory. But it may
be occurred. It is an example case that you dump your database.

Normally, you must have memory and swap that is larger than
database. Linux has tuning parameter to work with less memory and swap
than database size.

.. _tuning-linux:

Linux
-----

This section describes how to configure parameters on Linux.

.. _tuning-linux-nofile:

``nofile``
^^^^^^^^^^

You can relax the :ref:`tuning-max-n-open-files` parameter by creating
a configuration file ``/etc/security/limits.d/groonga.conf`` that has
the following content::

  ${USER} soft nofile ${MAX_VALUE}
  ${USER} hard nofile ${MAX_VALUE}

If you run Groonga process by ``groonga`` user and your Groonga
process needs to open less than 10000 files, use the following
configuration::

  groonga soft nofile 10000
  groonga hard nofile 10000

The configuration is applied after your Groonga service is restarted
or re-login as your ``groonga`` user.

.. _tuning-linux-overcommit-memory:

``vm.overcommit_memory``
^^^^^^^^^^^^^^^^^^^^^^^^

This is :ref:`tuning-memory-usage` related parameter. You can handle a
database that is larger than your memory and swap by setting
``vm.overcommit_memory`` kernel parameter to ``1``. ``1`` means that
Groonga can always map database files onto memory. Groonga recommends
the configuration.

See `Linux kernel documentation about overcommit
<https://www.kernel.org/doc/Documentation/vm/overcommit-accounting>`_
about ``vm.overcommit_memory`` parameter details.

You can set the configuration by putting a configuration file
``/etc/sysctl.d/groonga.conf`` that has the following content::

  vm.overcommit_memory = 1

The configuration can be applied by restarting your system or run the
following command::

  % sudo sysctl --system

.. _tuning-linux-max-map-count:

``vm.max_map_count``
^^^^^^^^^^^^^^^^^^^^

This is :ref:`tuning-memory-usage` related parameter. You can handle a
16GiB or more larger size database by increasing ``vm.max_map_count``
kernel parameter. The parameter limits the max number of memory maps.

The default value of the kernel parameter may be ``65530`` or
``65536``.  Groonga maps 256KiB memory chunk at one time. If a
database is larger than 16GiB, Groonga reaches the
limitation. (``256KiB * 65536 = 16GiB``)

You needs to increase the value of the kernel parameter to handle
16GiB or more larger size database. For example, you can handle almost
32GiB size database by ``65536 * 2 = 131072``. You can set the
configuration by putting a configuration file
``/etc/sysctl.d/groonga.conf`` that has the following content::

  vm.max_map_count = 131072

Note that your real configuration file will be the following because
you already have ``vm.overcommit_memory`` configuration::

  vm.overcommit_memory = 1
  vm.max_map_count = 131072

The configuration can be applied by restarting your system or run the
following command::

  % sudo sysctl -p

FreeBSD
-------

This section describes how to configure parameters on FreeBSD.

.. _tuning-freebsd-maxfilesperproc:

``kern.maxfileperproc``
^^^^^^^^^^^^^^^^^^^^^^^

TODO
