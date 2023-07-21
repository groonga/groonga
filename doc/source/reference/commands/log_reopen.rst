.. -*- rst -*-

``log_reopen``
==============

Summary
-------

``log_reopen`` is a command that reloads log files.

It is used to reload log files such as groonga log or query log which
are specified by ``--log-path`` or ``--query-log-path`` options.

.. note::

    This command only works when the number of worker processes is
    equal to 1.  Thus, it means that if you use
    :doc:`/reference/executables/groonga-httpd` with 2 or more
    workers, you must use ``groonga-httpd -s reopen`` instead.

Syntax
------

This command takes no parameters::

  log_reopen

Usage
-----
::

 log_reopen

 [true]

Lotate log files with `log_reopen`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Rename target log files such as mv command.
   (Log content is still written into moved log files)
2. Execute ``log_reopen`` command.
3. New log file is created as same as existing log file name.
   newer log content is written to new log file.

Parameters
----------

There is no required parameter.

Return value
------------

The command returns ``true`` as body if the command succeeds such as::

  [HEADER, true]

The command returns ``false`` otherwise such as::

  [HEADER, false]

See also
--------

:doc:`log_level`
:doc:`log_put`
