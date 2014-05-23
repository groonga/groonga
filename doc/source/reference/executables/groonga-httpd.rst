.. -*- rst -*-
.. Groonga Project

.. highlightlang:: none

groonga-httpd
=============

Summary
-------

groonga-httpd is a program to communicate with a Groonga server using
the HTTP protocol. It functions as same as
:doc:`groonga-server-http`. Although :doc:`groonga-server-http` has
limited support for HTTP with a minimal built-in HTTP server,
groonga-httpd has full support for HTTP with an embedded `nginx
<http://nginx.org/>`_. All standards-compliance and features provided
by nginx is also available in groonga-httpd.

groonga-httpd has an Web-based administration tool implemented with HTML and
JavaScript. You can access to it from http://hostname:port/.

Synopsis
--------

::

  groonga-httpd [nginx options]

Usage
-----

Set up
^^^^^^

First, you'll need to edit the groonga-httpd configuration file to specify a
database. Edit /etc/groonga/httpd/groonga-httpd.conf to enable the
``groonga_database`` directive like this::

   # Match this to the file owner of groonga database files if groonga-httpd is
   # run as root.
   #user groonga;
   ...
   http {
     ...
     # Don't change the location; currently only /d/ is supported.
     location /d/ {
       groonga on; # <= This means to turn on groonga-httpd.

       # Specify an actual database and enable this.
       groonga_database /var/lib/groonga/db/db;
     }
     ...
   }

Then, run groonga-httpd. Note that the control immediately returns back to the
console because groonga-httpd runs as a daemon process by default.::

   % groonga-httpd

Request queries
^^^^^^^^^^^^^^^

To check, request a simple query (:doc:`/reference/commands/status`).

.. groonga-command
.. database: groonga-httpd
.. include:: ../../example/reference/executables/groonga-httpd.log
.. /d/status


Loading data by POST
^^^^^^^^^^^^^^^^^^^^

The feature of groonga-httpd in contrast to groonga is supporting POST method.

You can load data by POST JSON data.

Here is an example curl command line that loads two users alice and bob to Users table::

    % curl --data-binary '[{"_key": "alice"}, {"_key": "bob"}]' -H "Content-Type: application/json" "http://localhost:10041/d/load?table=Users"

If you loads users from JSON file, prepare JSON file like this::

    [
    {"_key": "alice"},
    {"_key": "bob"}
    ]

Then specify JSON file in curl command line::

    % curl -X POST 'http://localhost:10041/d/load?table=Users' -H 'Content-Type: application/json' -d @users.json


Browse the administration tool
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Also, you can browse Web-based administration tool at http://localhost:10041/.

Shut down
^^^^^^^^^

Finally, to terminate the running groonga-httpd daemon, run this::

   % groonga-httpd -s stop

Configuration directives
------------------------

This section decribes only important directives. They are
groonga-httpd specific directives and performance related directives.

The following directives can be used in the groonga-httpd configuration file.
By default, it's located at /etc/groonga/httpd/groonga-httpd.conf.

Groonga-httpd specific directives
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following directives aren't provided by nginx. They are provided
by groonga-httpd to configure groonga-httpd specific configurations.

``groonga``
"""""""""""

Synopsis::

  groonga on | off;

Default
  ``groonga off;``

Context
  ``location``

Specifies whether Groonga is enabled in the ``location`` block. The
default is ``off``. You need to specify ``on`` to enable groonga.

Examples::

  location /d/ {
    groonga on;  # Enables groonga under /d/... path
  }

  location /d/ {
    groonga off; # Disables groonga under /d/... path
  }

.. _groonga-database:

``groonga_database``
""""""""""""""""""""

Synopsis::

  groonga_database /path/to/groonga/database;

Default
  ``groonga_database /usr/local/var/lib/groonga/db/db;``

Context
  ``http``, ``server``, ``location``

Specifies the path to a Groonga database. This is the required
directive.

.. _groonga-database-auto-create:

``groonga_database_auto_create``
""""""""""""""""""""""""""""""""

Synopsis::

  groonga_database_auto_create on | off;

Default
  ``groonga_database_auto_create on;``

Context
  ``http``, ``server``, ``location``

Specifies whether Groonga database is created automatically or not. If
the value is ``on`` and the Groonga database specified by
:ref:`groonga-database` doesn't exist, the Groonga database is created
automatically. If the Groonga database exists, groonga-httpd does
nothing.

If parent directory doesn't exist, parent directory is also created
recursively.

The default value is ``on``. Normally, the value doesn't need to be
changed.


``groonga_base_path``
"""""""""""""""""""""

Synopsis::

  groonga_base_path /d/;

Default
  The same value as ``location`` name.

Context
  ``location``

Specifies the base path in URI. Groonga uses
``/d/command?parameter1=value1&...`` path to run ``command``. The form
of path in used in groonga-httpd but groonga-httpd also supports
``/other-prefix/command?parameter1=value1&...`` form. To support the
form, groonga-httpd removes the base path from the head of request URI
and prepend ``/d/`` to the processed request URI. By the path
conversion, users can use custom path prefix and Groonga can always
uses ``/d/command?parameter1=value1&...`` form.

Nomally, this directive isn't needed. It is needed for per command
configuration.

Here is an example configuration to add authorization to
:doc:`/reference/commands/shutdown` command::

  groonga_database /var/lib/groonga/db/db;

  location /d/shutdown {
    groonga on;
    # groonga_base_path is needed.
    # Because /d/shutdown is handled as the base path.
    # Without this configuration, /d/shutdown/shutdown path is required
    # to run shutdown command.
    groonga_base_path /d/;
    auth_basic           "manager is required!";
    auth_basic_user_file "/etc/managers.htpasswd";
  }

  location /d/ {
    groonga on;
    # groonga_base_path doesn't needed.
    # Because location name is the base path.
  }

.. _groonga-log-path:

``groonga_log_path``
"""""""""""""""""""""

Synopsis::

  groonga_log_path path | off;

Default
  ``/var/log/groonga/httpd/groonga.log``

Context
  ``http``, ``server``, ``location``

Specifies Groonga log path in the ``http``, ``server`` or ``location`` block. The
default is ``/var/log/groonga/httpd/groonga.log``.
You can disable logging to specify ``off``.

Examples::

  location /d/ {
    groonga on;
    # You can disable log for groonga.
    groonga_log_path off;
  }

.. _groonga-log-level:

``groonga_log_level``
"""""""""""""""""""""

Synopsis::

  groonga_log_level none | emergency | alert | ciritical | error | warning | notice | info | debug | dump;

Default
  ``notice``

Context
  ``http``, ``server``, ``location``

Specifies Groonga log level in the ``http``, ``server`` or ``location`` block. The
default is ``notice``. You can disable logging by specifying ``none`` as log level.

Examples::

  location /d/ {
    groonga on;
    # You can customize log level for groonga.
    groonga_log_level notice;
  }

.. _groonga-query-log-path:

``groonga_query_log_path``
""""""""""""""""""""""""""

Synopsis::

  groonga_query_log_path path | off;

Default
  ``/var/log/groonga/httpd/groonga-query.log``

Context
  ``http``, ``server``, ``location``

Specifies Groonga's query log path in the ``http``, ``server`` or
``location`` block. The default is
``/var/log/groonga/httpd/groonga-query.log``.  You can disable logging
to specify ``off``.

Examples::

  location /d/ {
    groonga on;
    # You can disable query log for groonga.
    groonga_query_log_path off;
  }

Query log is useful for the following cases:

  * Detecting slow query.
  * Debugging.

You can analyze your query log by `groonga-query-log package
<https://github.com/groonga/groonga-query-log>`_. The package provides
useful tools.

For example, there is a tool that analyzing your query log. It can
detect slow queries from your query log. There is a tool that
replaying same queries in your query log. It can test the new Groonga
before updating production environment.

Performance related directives
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following directives are related to the performance of groonga-httpd.

``worker_processes``
""""""""""""""""""""

For optimum performance, set this to be equal to the number of CPUs or cores. In
many cases, Groonga queries may be CPU-intensive work, so to fully utilize
multi-CPU/core systems, it's essential to set this accordingly.

This isn't a groonga-httpd specific directive, but an nginx's one. For details,
see http://wiki.nginx.org/CoreModule#worker_processes.

By default, this is set to 1. It is nginx's default.

.. _groonga-cache-limit:

``groonga_cache_limit``
"""""""""""""""""""""""

This directive is introduced to customize cache limit for each worker process.

Synopsis::

  groonga_cache_limit CACHE_LIMIT;

Default
  100

Context
  ``http``, ``server``, ``location``

Specifies Groonga's limit of query cache in the ``http``, ``server`` or
``location`` block. The default value is 100.
You can disable query cache to specify 0 to ``groonga_cache_limit`` explicitly.

Examples::

  location /d/ {
    groonga on;
    # You can customize query cache limit for groonga.
    groonga_cache_limit 100;
  }

``proxy_cache``
"""""""""""""""

In short, you can use nginx's reverse proxy and cache mechanism
instead of Groonga's built-in query cache feature.

Query cache
+++++++++++

Groonga has query cache feature for :doc:`/reference/commands/select`
command. The feature improves performance in many cases.

Query cache feature works well on groonga-httpd except you use
:doc:`/reference/commands/cache_limit` command on 2 or more
workers. Normally, :doc:`/reference/commands/cache_limit` command
isn't used. So there is no problem on many cases.

Here is a description about a problem of using
:doc:`/reference/commands/cache_limit` command on 2 or more workers.

Groonga's query cache is available in the same process. It means that
workers can't share the cache. If you don't change cache size, it
isn't a big problem. If you want to change cache size by
:doc:`/reference/commands/cache_limit` command, there is a problem.

There is no portable ways to change cache size for all workers.

For example, there are 3 workers::

                                     +-- worker 1
  client -- groonga-httpd (master) --+-- worker 2
                                     +-- worker 3

The client requests :doc:`/reference/commands/cache_limit` command and
the worker 1 receives it::

                                     +-> worker 1 (changed!)
  client -> groonga-httpd (master) --+-- worker 2
                                     +-- worker 3

The client requests :doc:`/reference/commands/cache_limit` command
again and the worker 1 receives it again::

                                     +-> worker 1 (changed again!!!)
  client -> groonga-httpd (master) --+-- worker 2
                                     +-- worker 3

In this case, the worker 2 and the worker 3 aren't received any
requests. So they don't change cache size.

You can't choose a worker. So you can't change cache sizes of all
workers by :doc:`/reference/commands/cache_limit` command.

Reverse proxy and cache
+++++++++++++++++++++++

You can use nginx's reverse proxy and cache feature for query cache::

                                                              +-- worker 1
  client -- groonga-httpd (master) -- reverse proxy + cache --+-- worker 2
                                                              +-- worker 3

You can use the same cache configuration for all workers but you can't
change cache configuration dynamically by HTTP.

Here is a sample configuration::

  ...
  http {
    proxy_cache_path /var/cache/groonga-httpd levels=1:2 keys_zone=groonga:10m;
    proxy_cache_valid 10m;
    ...
    # Reverse proxy and cache
    server {
      listen 10041;
      ...
      # Only select command
      location /d/select {
        # Pass through groonga with cache
        proxy_cache groonga;
        proxy_pass http://localhost:20041;
      }

      location / {
        # Pass through groonga
        proxy_pass http://localhost:20041;
      }
    }

    # groonga
    server {
      location 20041;
      location /d/ {
        groonga on;
        groonga_database /var/lib/groonga/db/db;
      }
    }
    ...
  }

See the following nginx documentations for parameter details:

  * http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_cache_path
  * http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_cache_valid
  * http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_cache
  * http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_pass

Note that you need to remove cache files created by nginx by hand
after you load new data to Groonga. For the above sample
configuration, run the following command to remove cache files::

  % groonga DB_PATH < load.grn
  % rm -rf /var/cache/groonga-httpd/*

If you use Groonga's query cache feature, you don't need to expire
cache by hand. It is done automatically.

Available nginx modules
-----------------------

All standard HTTP modules are available. HttpRewriteModule is
disabled when you don't have PCRE (Perl Compatible Regular
Expressions). For the list of standard HTTP modules, see
http://wiki.nginx.org/Modules.
