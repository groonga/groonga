.. -*- rst -*-

.. highlightlang:: none

Server packages
===============

The package ``groonga`` is the mininum set of fulltext search engine.
If you want to use groonga for server use, you can install
additional preconfigured packages.

There are three packages for server use.

* ``groonga-httpd`` (nginx and HTTP protocol based server package)
* ``groonga-server-http`` (simple HTTP protocol based server package)
* ``groonga-server-gqtp`` (:doc:`/spec/gqtp` protocol based server package)

There is the reason why groonga supports not only GQTP but also two HTTP server
packages. :doc:`/spec/gqtp` - Groonga Query Transfer Protocol is desined to reduce overheads
and improve performance. But, GQTP is less support of client library than HTTP protocol does.
As HTTP is matured protocol, you can take advantage of existing tool and there are many client
library (See `related projects <http://groonga.org/related-projects.html>`_ for details).
If you use ``groonga-httpd`` package, you can also take benefits of nginx functionality.

We recommend to use ``groonga-server-http`` at first, then if you want to use more 
fullfilling functionality, use ``groonga-httpd`` package. If you have performance issues which
is derived from protocol overheads, consider to use ``groonga-server-gqtp``.

 .. note::

   There is a conflict between ``groonga-httpd`` and ``groonga-server-http``
   package because of port number.
   See :doc:`/server/http/comparison` if you want to know which package meets your demands.

groonga-httpd
-------------

``groonga-httpd`` is a nginx and HTTP protocol based server package.

Preconfigured setting:

+--------------------+---------------------------------------+
| Item               | Default value                         |
+====================+=======================================+
| Port number        | 10041                                 |
+--------------------+---------------------------------------+
| Access log path    | /var/log/groonga/httpd/acccess.log    |
+--------------------+---------------------------------------+
| Error log path     | /var/log/groonga/http-query.log       |
+--------------------+---------------------------------------+
| Database           | /var/lib/groonga/db/*                 |
+--------------------+---------------------------------------+
| Configuration file | /etc/groonga/httpd/groonga-httpd.conf |
+--------------------+---------------------------------------+

Start HTTP server
^^^^^^^^^^^^^^^^^

Starting groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-httpd start
 
Starting groonga HTTP server(Fedora)::

  % sudo systemctl start groonga-httpd
 
Stop HTTP server
^^^^^^^^^^^^^^^^

Stopping groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-httpd stop
 
Starting groonga HTTP server(Fedora)::

  % sudo systemctl stop groonga-httpd

Restart HTTP server
^^^^^^^^^^^^^^^^^^^

Restarting groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-httpd restart
 
Restarting groonga HTTP server(Fedora)::

  % sudo systemctl restart groonga-httpd

groonga-server-http
-------------------

``groonga-server-http`` is a simple HTTP protocol based server package.

Preconfigured setting:

+--------------------+---------------------------------------+
| Item               | Default value                         |
+====================+=======================================+
| Port number        | 10041                                 |
+--------------------+---------------------------------------+
| :ref:`process-log` | /var/log/groonga/groonga-http.log     |
+--------------------+---------------------------------------+
| :ref:`query-log`   | /var/log/groonga/http-query.log       |
+--------------------+---------------------------------------+
| Database           | /var/lib/groonga/db/*                 |
+--------------------+---------------------------------------+

Configuration file for server setting (Debian/Ubuntu)::

  /etc/default/groonga/groonga-server-http

Configuration file for server setting (CentOS)::

  /etc/sysconfig/groonga-server-http

Start HTTP server
^^^^^^^^^^^^^^^^^

Starting groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-http start
 
Starting groonga HTTP server(Fedora)::

  % sudo systemctl start groonga-server-http
 
Stop HTTP server
^^^^^^^^^^^^^^^^

Stopping groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-http stop
 
Stopping groonga HTTP server(Fedora)::

  % sudo systemctl stop groonga-server-http

Restart HTTP server
^^^^^^^^^^^^^^^^^^^

Restarting groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-http restart
 
Restarting groonga HTTP server(Fedora)::

  % sudo systemctl restart groonga-server-http

groonga-server-gqtp
-------------------

``groonga-server-gqtp`` is a :doc:`/spec/gqtp` protocol based server package.

+--------------------+---------------------------------------+
| Item               | Default value                         |
+====================+=======================================+
| Port number        | 10043                                 |
+--------------------+---------------------------------------+
| :ref:`process-log` | /var/log/groonga/groonga-gqtp.log     |
+--------------------+---------------------------------------+
| :ref:`query-log`   | /var/log/groonga/gqtp-query.log       |
+--------------------+---------------------------------------+
| Database           | /var/lib/groonga/db/*                 |
+--------------------+---------------------------------------+

Configuration file for server setting (Debian/Ubuntu)::

  /etc/default/groonga/groonga-server-gqtp

Configuration file for server setting (CentOS)::

  /etc/sysconfig/groonga-server-gqtp

Start GQTP server
^^^^^^^^^^^^^^^^^

Starting groonga GQTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-gqtp start
 
Starting groonga GQTP server(Fedora)::

  % sudo systemctl start groonga-server-gqtp

Stop GQTP server
^^^^^^^^^^^^^^^^

Stopping groonga GQTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-http stop
 
Stopping groonga GQTP server(Fedora)::

  % sudo systemctl stop groonga-server-gqtp

Restart GQTP server
^^^^^^^^^^^^^^^^^^^

Restarting groonga HTTP server(Debian/Ubuntu/CentOS)::

  % sudo service groonga-server-gqtp restart
 
Restarting groonga HTTP server(Fedora)::

  % sudo systemctl restart groonga-server-gqtp
