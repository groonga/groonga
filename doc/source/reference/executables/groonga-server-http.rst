.. -*- rst -*-

Groonga HTTP server
===================

Summary
-------

You can communicate by HTTP if you specify ``http`` to ``--protocol`` option. And output a file that is put under the path, and correspond to specified URI to HTTP request if you specify static page path by ``--document-root``.

Groonga has an Web-based administration tool implemented with HTML and JavaScript. If you don't specify ``--document-root``, regarded as administration tool installed path is specified, so you can use administration tool to access ``http://HOSTNAME:PORT/`` in Web browser.

Syntax
------

You must specify ``--protocol http``::

  groonga --protocol http -d [options...] DB_PATH

Usage
-----

You can use HTTP GET or HTTP POST to send a request. One request runs
only one command. You can't run multiple commands by one request.

You must use ``/d/${COMMAND_NAME}`` path for request.

Here is an example URL to run :doc:`/reference/commands/status`::

  http://127.0.0.1:10041/d/status

If you use HTTP GET, you must specify parameters as URL "query".

Here is an example URL to specify ``3`` as ``command_version``::

  http://127.0.0.1:10041/d/status?command_version=3

You can also specify multiple parameters::

  http://127.0.0.1:10041/d/status?command_version=3&output_pretty=yes

If you use HTTP POST, you can specify parameters by URL "query" and
HTTP request body. If you use HTTP request body, you must specify
``application/x-www-form-urlencoded`` as ``Content-Type`` header
value.

Here is an example HTTP POST request to specify multiple parameters by
HTTP request body::

  POST /d/status HTTP/1.1
  Host: 127.0.0.1:10041
  Content-Length: 35
  Content-Type: application/x-www-form-urlencoded

  command_version=3&output_pretty=yes

You can mix URL "query" and HTTP request body::

  POST /d/status?command_version=3 HTTP/1.1
  Host: 127.0.0.1:10041
  Content-Length: 17
  Content-Type: application/x-www-form-urlencoded

  output_pretty=yes

You can also use HTTP POST to specify data for
:doc:`/reference/commands/load`. If you send data by HTTP POST, you
can't specify parameters as HTTP body. You must specify parameters by
URL "path".

You must specify suitable HTTP ``Content-Type`` header value and
:doc:`/reference/commands/load` ``input_type`` parameter value for
your data. Here are available values:

.. list-table::
   :header-rows: 1

   * - ``Content-Type``
     - ``input_type``
     - Description
   * - ``application/json``
     - ``json``
     - Send JSON data.
   * - ``application/x-apache-arrow-streaming``
     - ``apache-arrow``
     - Send Apache Arrow data.

You can specify :doc:`/reference/command/output_format` as URL
"path" extension.

Here is an example HTTP request to get response as JSON::

  http://127.0.0.1:10041/d/status.json

Here is an example HTTP request to get response as XML::

  http://127.0.0.1:10041/d/status.xml

See also
--------

  * :doc:`groonga-httpd`

  * :doc:`/reference/commands/load`
