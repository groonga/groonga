.. -*- rst -*-

.. highlightlang:: none

Groonga HTTP server
===================

Name
----

Groonga HTTP server

Synopsis
--------

::

 groonga -d --protocol http DB_PATH

Summary
-------

You can communicate by HTTP if you specify ``http`` to ``--protocol`` option. And output a file that is put under the path, and correspond to specified URI to HTTP request if you specify static page path by ``--document-root``.

Groonga has an Web-based administration tool implemented with HTML and JavaScript. If you don't specify ``--document-root``, regarded as administration tool installed path is specified, so you can use administration tool to access ``http://HOSTNAME:PORT/`` in Web browser.

Command
-------

You can use the same commands of Groonga that starts of the other mode to Groonga server that starts to specify ``http``.

A command takes the arguments. An argument has a name. And there are special arguments ``output_type`` and ``command_version``.

In standalone mode or client mode, a command is specified by the following format.

 Format 1: COMMAND_NAME VALUE1 VALUE2,..

 Format 2: COMMAND_NAME --PARAMETER_NAME1 VALUE1 --PARAMETER_NAME2 VALUE2,..

Format 1 and Format 2 are possible to mix. Output type is specified by ``output_type`` in the formats.

In HTTP server mode, the following formats to specify command::

 Format: /d/COMMAND_NAME.OUTPUT_TYPE?ARGUMENT_NAME1=VALUE1&ARGUMENT_NAME2=VALUE2&...

But, they need URL encode for command names, arguments names and values.

You can use GET method only.

You can specify JSON, TSV and XML to output type.

``command_version`` is specified for command specification compatibility. See :doc:`/reference/command/command_version` for details.

Return value
------------

The execution result is output that follows output type specification by the command.
