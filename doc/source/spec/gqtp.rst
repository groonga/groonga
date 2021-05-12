.. -*- rst -*-

GQTP
====

GQTP is the acronym of Groonga Query Transfer Protocol. GQTP is the
original protocol for groonga.

Protocol
--------

GQTP is stateful client server model protocol. The following sequence
is one processing unit:

* Client sends a request
* Server receives the request
* Server processes the request
* Server sends a response
* Client receives the response

You can do zero or more processing units in a session.

Both request and response consist of GQTP header and body. GQTP header
is fixed size data. Body is variable size data and its size is stored
in GQTP header. The content of body isn't defined in GQTP.

.. _gqtp-header-spec:

GQTP header
^^^^^^^^^^^

GQTP header consists of the following unsigned integer values:

+----------------+-------+----------------------------+
|      Name      |  Size |        Description         |
+================+=======+============================+
|  ``protocol``  | 1byte | Protocol type.             |
+----------------+-------+----------------------------+
| ``query_type`` | 1byte | Content type of body.      |
+----------------+-------+----------------------------+
| ``key_length`` | 2byte | Not used.                  |
+----------------+-------+----------------------------+
|   ``level``    | 1byte | Not used.                  |
+----------------+-------+----------------------------+
|   ``flags``    | 1byte | Flags.                     |
+----------------+-------+----------------------------+
|   ``status``   | 2byte | Return code.               |
+----------------+-------+----------------------------+
|    ``size``    | 4byte | Body size.                 |
+----------------+-------+----------------------------+
|   ``opaque``   | 4byte | Not used.                  |
+----------------+-------+----------------------------+
|    ``cas``     | 8byte | Not used.                  |
+----------------+-------+----------------------------+

All header values are encoded by network byte order.

The following sections describes available values of each header value.

The total size of GQTP header is 24byte.

``protocol``
""""""""""""

The value is always ``0xc7`` in both request and response GQTP header.

``query_type``
""""""""""""""

The value is one of the following values:

+-------------+--------+----------------------------+
|    Name     | Value  |        Description         |
+=============+========+============================+
|  ``NONE``   |   0    | Free format.               |
+-------------+--------+----------------------------+
|   ``TSV``   |   1    | Tab Separated Values.      |
+-------------+--------+----------------------------+
|  ``JSON``   |   2    | JSON.                      |
+-------------+--------+----------------------------+
|   ``XML``   |   3    | XML.                       |
+-------------+--------+----------------------------+
| ``MSGPACK`` |   4    | MessagePack.               |
+-------------+--------+----------------------------+

This is not used in request GQTP header.

This is used in response GQTP header. Body is formatted as specified
type.

``flags``
"""""""""

The value is bitwise OR of the following values:

+-----------+--------+----------------------------+
|   Name    | Value  |        Description         |
+===========+========+============================+
| ``MORE``  |  0x01  | There are more data.       |
+-----------+--------+----------------------------+
| ``TAIL``  |  0x02  | There are no more data.    |
+-----------+--------+----------------------------+
| ``HEAD``  |  0x04  | Not used.                  |
+-----------+--------+----------------------------+
| ``QUIET`` |  0x08  | Be quiet.                  |
+-----------+--------+----------------------------+
| ``QUIT``  |  0x10  | Quit.                      |
+-----------+--------+----------------------------+

You must specify ``MORE`` or ``TAIL`` flag.

If you use ``MORE`` flag, you should also use ``QUIET`` flag. Because
you don't need to show a response for your partial request.

Use ``QUIT`` flag to quit this session.

``status``
""""""""""

Here are available values. The new statuses will be added in the
future.

* 0: ``SUCCESS``
* 1: ``END_OF_DATA``
* 65535: ``UNKNOWN_ERROR``
* 65534: ``OPERATION_NOT_PERMITTED``
* 65533: ``NO_SUCH_FILE_OR_DIRECTORY``
* 65532: ``NO_SUCH_PROCESS``
* 65531: ``INTERRUPTED_FUNCTION_CALL``
* 65530: ``INPUT_OUTPUT_ERROR``
* 65529: ``NO_SUCH_DEVICE_OR_ADDRESS``
* 65528: ``ARG_LIST_TOO_LONG``
* 65527: ``EXEC_FORMAT_ERROR``
* 65526: ``BAD_FILE_DESCRIPTOR``
* 65525: ``NO_CHILD_PROCESSES``
* 65524: ``RESOURCE_TEMPORARILY_UNAVAILABLE``
* 65523: ``NOT_ENOUGH_SPACE``
* 65522: ``PERMISSION_DENIED``
* 65521: ``BAD_ADDRESS``
* 65520: ``RESOURCE_BUSY``
* 65519: ``FILE_EXISTS``
* 65518: ``IMPROPER_LINK``
* 65517: ``NO_SUCH_DEVICE``
* 65516: ``NOT_A_DIRECTORY``
* 65515: ``IS_A_DIRECTORY``
* 65514: ``INVALID_ARGUMENT``
* 65513: ``TOO_MANY_OPEN_FILES_IN_SYSTEM``
* 65512: ``TOO_MANY_OPEN_FILES``
* 65511: ``INAPPROPRIATE_I_O_CONTROL_OPERATION``
* 65510: ``FILE_TOO_LARGE``
* 65509: ``NO_SPACE_LEFT_ON_DEVICE``
* 65508: ``INVALID_SEEK``
* 65507: ``READ_ONLY_FILE_SYSTEM``
* 65506: ``TOO_MANY_LINKS``
* 65505: ``BROKEN_PIPE``
* 65504: ``DOMAIN_ERROR``
* 65503: ``RESULT_TOO_LARGE``
* 65502: ``RESOURCE_DEADLOCK_AVOIDED``
* 65501: ``NO_MEMORY_AVAILABLE``
* 65500: ``FILENAME_TOO_LONG``
* 65499: ``NO_LOCKS_AVAILABLE``
* 65498: ``FUNCTION_NOT_IMPLEMENTED``
* 65497: ``DIRECTORY_NOT_EMPTY``
* 65496: ``ILLEGAL_BYTE_SEQUENCE``
* 65495: ``SOCKET_NOT_INITIALIZED``
* 65494: ``OPERATION_WOULD_BLOCK``
* 65493: ``ADDRESS_IS_NOT_AVAILABLE``
* 65492: ``NETWORK_IS_DOWN``
* 65491: ``NO_BUFFER``
* 65490: ``SOCKET_IS_ALREADY_CONNECTED``
* 65489: ``SOCKET_IS_NOT_CONNECTED``
* 65488: ``SOCKET_IS_ALREADY_SHUTDOWNED``
* 65487: ``OPERATION_TIMEOUT``
* 65486: ``CONNECTION_REFUSED``
* 65485: ``RANGE_ERROR``
* 65484: ``TOKENIZER_ERROR``
* 65483: ``FILE_CORRUPT``
* 65482: ``INVALID_FORMAT``
* 65481: ``OBJECT_CORRUPT``
* 65480: ``TOO_MANY_SYMBOLIC_LINKS``
* 65479: ``NOT_SOCKET``
* 65478: ``OPERATION_NOT_SUPPORTED``
* 65477: ``ADDRESS_IS_IN_USE``
* 65476: ``ZLIB_ERROR``
* 65475: ``LZO_ERROR``
* 65474: ``STACK_OVER_FLOW``
* 65473: ``SYNTAX_ERROR``
* 65472: ``RETRY_MAX``
* 65471: ``INCOMPATIBLE_FILE_FORMAT``
* 65470: ``UPDATE_NOT_ALLOWED``
* 65469: ``TOO_SMALL_OFFSET``
* 65468: ``TOO_LARGE_OFFSET``
* 65467: ``TOO_SMALL_LIMIT``
* 65466: ``CAS_ERROR``
* 65465: ``UNSUPPORTED_COMMAND_VERSION``

``size``
""""""""

The size of body. The maximum body size is 4GiB because ``size`` is
4byte unsigned integer. If you want to send 4GiB or more larger data,
use ``MORE`` flag.

Example
-------

How to run a GQTP server
^^^^^^^^^^^^^^^^^^^^^^^^

Groonga has a special protocol, named Groonga Query Transfer Protocol (GQTP), for remote access to a database. The following form shows how to run Groonga as a GQTP server.

Form::

  groonga [-p PORT_NUMBER] -s DB_PATH

The `-s` option specifies to run Groonga as a server. DB_PATH specifies the path of the existing database to be hosted. The `-p` option and its argument, PORT_NUMBER, specify the port number of the server. The default port number is 10043, which is used when you don't specify PORT_NUMBER.

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

This command establishes a connection with a GQTP server and then enters into interactive mode. HOST_NAME_OR_IP_ADDRESS specifies the hostname or the IP address of the server. If not specified, Groonga uses the default hostname "localhost". The `-p` option and its argument, PORT_NUMBER, specify the port number of the server. If not specified, Groonga uses the default port number 10043.

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

See also
--------

* :doc:`/reference/executables/groonga`
* :doc:`/server/gqtp`
