.. -*- rst -*-

.. groonga-include : introduction.rst

.. groonga-command
.. database: tutorial

Remote access
=============

You can use Groonga as a server which allows remote access. Groonga supports the original protocol (GQTP), the memcached binary protocol and HTTP.

Hypertext transfer protocol (HTTP)
-----------------------------------

How to run an HTTP server
^^^^^^^^^^^^^^^^^^^^^^^^^

Groonga supports the hypertext transfer protocol (HTTP). The following form shows how to run Groonga as an HTTP server daemon.

Form::

  groonga [-p PORT_NUMBER] -d --protocol http DB_PATH

The `--protocol` option and its argument specify the protocol of the server. "http" specifies to use HTTP. If the `-p` option is not specified, Groonga uses the default port number 10041.

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

An HTTP server of Groonga provides a browser based administration tool that makes database management easy. After starting an HTTP server, you can use the administration tool by accessing ``http://HOST_NAME_OR_IP_ADDRESS[:PORT_NUMBER]/``. Note that JavaScript must be enabled for the tool to work properly.

Security issues
---------------

Groonga servers don't support user authentication. Everyone can view and modify databases hosted by Groonga servers. You are recommended to restrict IP addresses that can access Groonga servers. You can use iptables or similar for this purpose.
