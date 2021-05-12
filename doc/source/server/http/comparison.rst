.. -*- rst -*-

Comparison
==========

There are many differences between :doc:`groonga` and
:doc:`groonga-httpd`. Here is a comparison table.

+----------------------------+------------------------+----------------------+
|                            |        groonga         |    groonga-httpd     |
+============================+========================+======================+
|        Performance         | o                      | o                    |
+----------------------------+------------------------+----------------------+
|   Using multi CPU cores    | o (by multi threading) | o (by multi process) |
+----------------------------+------------------------+----------------------+
|     Configuration file     | optional               | required             |
+----------------------------+------------------------+----------------------+
|     Custom prefix path     | x                      | o                    |
+----------------------------+------------------------+----------------------+
|   Custom command version   | o                      | o                    |
+----------------------------+------------------------+----------------------+
|      Multi databases       | x                      | o                    |
+----------------------------+------------------------+----------------------+
|       Authentication       | x                      | o                    |
+----------------------------+------------------------+----------------------+
|      Gzip compression      | x                      | o                    |
+----------------------------+------------------------+----------------------+
|            POST            | o                      | o                    |
+----------------------------+------------------------+----------------------+
|           HTTPS            | x                      | o                    |
+----------------------------+------------------------+----------------------+
|         Access log         | x                      | o                    |
+----------------------------+------------------------+----------------------+
| Upgrading without downtime | x                      | o                    |
+----------------------------+------------------------+----------------------+

Performance
-----------

Both :doc:`groonga` and :doc:`groonga-httpd` are very fast. They can
work with the same throughput.

Using multi CPU cores
---------------------

Groonga scales on multi CPU cores. :doc:`groonga` scales by multi
threading. :doc:`groonga-httpd` scales by multi processes.

:doc:`groonga` uses the same number of threads as CPU cores by
default. If you have 8 CPU cores, 8 threads are used by default.

:doc:`groonga-httpd` uses 1 process by default. You need to set
`worker_processes
<http://nginx.org/en/docs/ngx_core_module.html#worker_processes>`_
directive to use CPU cores. If you have 8 CPU cores, specify
``worker_processes 8`` in configuration file like the following::

  worker_processes 8;

  http {
    # ...
  }

Configuration file
------------------

:doc:`groonga` can work without configuration file. All configuration
items such as port number and the max number of threads can be
specified by command line. Configuration file is also used to specify
configuration items.

It's very easy to run groonga HTTP server because :doc:`groonga`
requires just a few options to run. Here is the most simple command
line to start HTTP server by :doc:`groonga`::

  % groonga --protocol http -d /PATH/TO/DATABASE

:doc:`groonga-httpd` requires configuration file to run. Here is the
most simple configuration file to start HTTP server by
:doc:`groonga-httpd`::

  events {
  }

  http {
    server {
      listen 10041;

      location /d/ {
        groonga on;
        groonga_database /PATH/TO/DATABASE;
      }
    }
  }

Custom prefix path
------------------

:doc:`groonga` accepts a path that starts with ``/d/`` as command URL
such as ``http://localhost:10041/d/status``. You cannot change the
prefix path ``/d/``.

:doc:`groonga-httpd` can custom prefix path. For example, you can use
``http://localhost:10041/api/status`` as command URL. Here is a sample
configuration to use ``/api/`` as prefix path::

  events {
  }

  http {
    server {
      listen 10041;

      location /api/ { # <- change this
        groonga on;
        groonga_database /PATH/TO/DATABASE;
      }
    }
  }

Custom command version
----------------------

Groonga has :doc:`/reference/command/command_version` mechanism. It is for
upgrading groonga commands with backward compatibility.

:doc:`groonga` can change the default command veresion by
``--default-command-version`` option. Here is a sample command line to
use command version 2 as the default command version::

  % groonga --protocol http --default-command-version 2 -d /PATH/TO/DATABASE

:doc:`groonga-httpd` cannot custom the default command version
yet. But it will be supported soon. If it is supported, you can
provides different command version groonga commands in the same
:doc:`groonga-httpd` process. Here is a sample configuration to
provide command version 1 commands under ``/api/1/`` and command
version 2 comamnds under ``/api/2/``::

  events {
  }

  http {
    server {
      listen 10041;

      groonga_database /PATH/TO/DATABASE;

      location /api/1/ {
        groonga on;
        groonga_default_command_version 1;
      }

      location /api/2/ {
        groonga on;
        groonga_default_command_version 2;
      }
    }
  }

Multi databases
---------------

:doc:`groonga` can use only one database in a process.

:doc:`groonga-httpd` can use one or more databases in a process. Here
is a sample configuration to provide ``/tmp/db1`` database under
``/db1/`` path and ``/tmp/db2`` database under ``/db2/`` path::

  events {
  }

  http {
    server {
      listen 10041;

      location /db1/ {
        groonga on;
        groonga_database /tmp/db1;
      }

      location /db2/ {
        groonga on;
        groonga_database /tmp/db2;
      }
    }
  }

Authentication
--------------

HTTP supports authentications such as basic authentication and digest
authentication. It can be used for restricting use of danger command such
as :doc:`/reference/commands/shutdown`.

:doc:`groonga` doesn't support any authentications. To restrict use of
danger command, other tools such as iptables and reverse proxy are
needed.

:doc:`groonga-httpd` supports basic authentication. Here is a sample
configuration to restrict use of :doc:`/reference/commands/shutdown`
command::

  events {
  }

  http {
    server {
      listen 10041;

      groonga_database /PATH/TO/DATABASE;

      location /d/shutdown {
        groonga on;
        auth_basic           "manager is required!";
        auth_basic_user_file "/etc/managers.htpasswd";
      }

      location /d/ {
        groonga on;
      }
    }
  }

Gzip compression
----------------

HTTP supports response compression by gzip with ``Content-Encoding:
gzip`` response header. It can reduce network flow. It is useful
for large search response.

:doc:`groonga` doesn't support compression. To support compression,
reverse proxy is needed.

:doc:`groonga-httpd` supports gzip compression. Here is a sample
configuration to compress response by gzip::

  events {
  }

  http {
    server {
      listen 10041;

      groonga_database /PATH/TO/DATABASE;

      location /d/ {
        groonga    on;
        gzip       on;
        gzip_types *;
      }
    }
  }

Note that `gzip_types *` is specified. It's one of the important
configuration. `gzip_types` specifies gzip target data formats by MIME
types. :doc:`groonga-httpd` returns one of JSON, XML or MessagePack
format data. But those formats aren't included in the default value of
`gzip_types`. The default value of `gzip_types` is `text/html`.

To compress response data from :doc:`groonga-httpd` by gzip, you need
to specify `gzip_types *` or `gzip_types application/json text/xml
application/x-msgpack` explicitly. `gzip_types *` is recommended.
There are two reasons for it. The first, groonga may support more
formats in the future. The second, all requests for the `location` are
processed by groonga. You don't need to consider about other modules.

POST
----

You can load your data by POST JSON data. You need follow the
following rules to use loading by POST.

* `Content-Type` header value must be `application/json`.
* JSON data is sent as body.
* Table name is specified by query parameter such as ``table=NAME``.

Here is an example curl command line that loads two users `alice` and
`bob` to `Users` table::

  % curl --data-binary '[{"_key": "alice"}, {"_key": "bob"}]' -H "Content-Type: application/json" "http://localhost:10041/d/load?table=Users"

HTTPS
-----

TODO

Access log
----------

TODO

Upgrading without downtime
--------------------------

TODO
