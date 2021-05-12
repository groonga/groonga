.. -*- rst -*-

GQTP
====

Summary
-------

GQTP is the acronym standing for "Groonga Query Transfer Protocol".

GQTP is a protocol designed for Groonga. It's a stateful
protocol. You can send multiple commands in one session.

GQTP will be faster rather than :doc:`/server/http` when you send many
light commands like :doc:`/reference/commands/status`. GQTP will be
almost same performance as HTTP when you send heavy commands like
:doc:`/reference/commands/select`.

We recommend that you use HTTP for many cases. Because there are many
HTTP client libraries.

If you want to use GQTP, you can use the following libraries:

  * Ruby: `groonga-client <https://github.com/ranguba/groonga-client>`_
  * Python: `poyonga <https://github.com/hhatto/poyonga>`_
  * Go: `goroo <https://github.com/hhatto/goroo>`_
  * PHP: `proonga <https://github.com/Yujiro3/proonga>`_
  * C/C++: Groonga (Groonga can be also used as library)

It's not a library but you can use
:doc:`/reference/executables/groonga` as a GQTP client.

How to run
----------

:doc:`/reference/executables/groonga` is a GQTP server implementation.
You can run a Groonga server by the following command line::

  groonga --protocol gqtp -s [options] DB_PATH

You can run a Groonga server as a daemon by the following command
line::

  groonga --protocol gqtp -d [options] DB_PATH

See :doc:`/reference/executables/groonga` for available ``options``.
