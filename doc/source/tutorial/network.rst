.. -*- rst -*-

.. highlightlang:: none

.. groonga-include : introduction.txt

.. groonga-command
.. database: tutorial

Remote access
=============

You can use Groonga as a server which allows remote access. Groonga supports the original protocol (GQTP), the memcached binary protocol and HTTP.

Groonga Query Transfer Protocol (GQTP)
--------------------------------------

How to run a GQTP server
^^^^^^^^^^^^^^^^^^^^^^^^

Groonga has a special protocol, named Groonga Query Transfer Protocol (GQTP), for remote access to a database. The following form shows how to run Groonga as a GQTP server.

Form::

  groonga [-p PORT_NUMBER] -s DB_PATH

The `-s` option specifies to run Groonga as a server. DB_PATH specifies the path of the existing database to be hosted. The `-p` option and its argument, PORT_NUMBER, specify the port number of the server. The default port number is 10041, which is used when you don't specify PORT_NUMBER.

The following command runs a server that listens on the default port number. The server accepts operations to the specified database.

Execution example::

  % groonga -s /tmp/groonga-databases/introduction.db
  Ctrl-c
  %

How to run a GQTP daemon
^^^^^^^^^^^^^^^^^^^^^^^^

You can also run a GQTP server as a daemon by using the `-d` option, instead of the `-s` option.

Form::

  groonga [-p PORT_NUMBER] -d DB_PATH

A Groonga daemon prints its process ID as follows. In this example, the process ID is 12345. Then, the daemon opens a specified database and accepts operations to that database.

Execution example::

  % groonga -d /tmp/groonga-databases/introduction.db
  12345
  %

How to run a GQTP client
^^^^^^^^^^^^^^^^^^^^^^^^

You can run Groonga as a GQTP client as follows:

Form::

  groonga [-p PORT_NUMBER] -c [HOST_NAME_OR_IP_ADDRESS]

This command establishes a connection with a GQTP server and then enters into interactive mode. HOST_NAME_OR_IP_ADDRESS specifies the hostname or the IP address of the server. If not specified, Groonga uses the default hostname "localhost". The `-p` option and its argument, PORT_NUMBER, specify the port number of the server. If not specified, Groonga uses the default port number 10041.

.. groonga-command
.. include:: ../example/tutorial/network-1.log
.. .. % groonga -c
.. status
.. .. > ctrl-d
.. .. %

In interactive mode, Groonga reads commands from the standard input and executes them one by one.

How to terminate a GQTP server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can terminate a GQTP server with a :doc:`/reference/commands/shutdown` command.

.. groonga-command
.. include:: ../example/tutorial/network-2.log
.. .. % groonga -c
.. .. > shutdown
.. .. %

Memcached binary protocol
-------------------------

Groonga supports the memcached binary protocol. The following form shows how to run Groonga as a memcached binary protocol server daemon.

Form::

  groonga [-p PORT_NUMBER] -d --protocol memcached DB_PATH

The `--protocol` option and its argument specify the protocol of the server. "memcached" specifies to use the memcached binary protocol.

Hypertext transfer protocol (HTTP)
-----------------------------------

How to run an HTTP server
^^^^^^^^^^^^^^^^^^^^^^^^^

Groonga supports the hypertext transfer protocol (HTTP). The following form shows how to run Groonga as an HTTP server daemon.

Form::

  groonga [-p PORT_NUMBER] -d --protocol http DB_PATH

The `--protocol` option and its argument specify the protocol of the server. "http" specifies to use HTTP.

The following command runs an HTTP server that listens on the port number 80.

Execution example::

  % sudo groonga -p 80 -d --protocol http /tmp/groonga-databases/introduction.db
  %

.. note::
   You must have root privileges if you listen on the port number 80 (well known port).
   There is no such a limitation about the port number 1024 or over.

How to send a command to an HTTP server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can send a command to an HTTP server by sending a GET request to /d/COMMAND_NAME. Command parameters can be passed as parameters of the GET request. The format is "?NAME_1=VALUE_1&NAME_2=VALUE_2&...".

The following example shows how to send commands to an HTTP server.

.. groonga-command
.. include:: ../example/tutorial/network-3.log
.. .. http://HOST_NAME_OR_IP_ADDRESS[:PORT_NUMBER]/d/status
.. .. Executed command:
.. status
.. .. http://HOST_NAME_OR_IP_ADDRESS[:PORT_NUMBER]/d/select?table=Site&query=title:@this
.. .. Executed command:
.. select --table Site --query title:@this

Administration tool (HTTP)
--------------------------

An HTTP server of Groonga provides a browser based administration tool that makes database management easy. After starting an HTTP server, you can use the administration tool by accessing http://HOST_NAME_OR_IP_ADDRESS[:PORT_NUMBER]/. Note that Javascript must be enabled for the tool to work properly.

Security issues
---------------

Groonga servers don't support user authentication. Everyone can view and modify databases hosted by Groonga servers. You are recommended to restrict IP addresses that can access Groonga servers. You can use iptables or similar for this purpose.
