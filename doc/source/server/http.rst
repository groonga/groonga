.. -*- rst -*-

HTTP
====

Groonga provides two HTTP server implementations.

* :doc:`http/groonga`
* :doc:`http/groonga-httpd`

:doc:`http/groonga` is a simple implemntation. It is fast but doesn't
have many HTTP features. It is convenient to try Groonga because it
requires just a few command line options to run.

:doc:`http/groonga-httpd` is a `nginx <http://nginx.org/>`_ based
implementation. It is also fast and has many HTTP features.

.. toctree::
   :maxdepth: 2

   http/comparison
   http/groonga
   http/groonga-httpd
