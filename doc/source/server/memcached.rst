.. -*- rst -*-

Memcached binary protocol
=========================

Groonga supports the memcached binary protocol. The following form shows how to run Groonga as a memcached binary protocol server daemon.

Form::

  groonga [-p PORT_NUMBER] -d --protocol memcached DB_PATH

The `--protocol` option and its argument specify the protocol of the server. "memcached" specifies to use the memcached binary protocol.

You don't need to create a table. When Groonga receives a request, it creates a table automatically. The table name will be `Memcache` .
