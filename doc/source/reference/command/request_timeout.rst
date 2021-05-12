.. -*- rst -*-

Request timeout
===============

Summary
-------

.. versionadded:: 6.0.2

You can set timeout to each request.

If a request isn't completed until the specified timeout, the request
is canceled. If you don't set timeout, the request is processed until
the request is completed.

Request timeout feature is useful when you implement timeout on client
side. If you only implement timeout only on client side, the request
keeps processing after client stops waiting response. It uses needless
resources. If you set timeout to the request, the request will be
canceled soon.

How to set timeout to request
-----------------------------

All commands accept ``request_timeout`` parameter. You can set timeout
to request by adding ``request_timeout`` parameter.

Unit of timeout out value is second. You can set timeout less than 1
second by using decimal such as ``0.1``. ``0.1`` means 100
milliseconds.

Here is an example to set ``5.5`` seconds timeout to a request::

  select Users --request_timeout 5.5

Return code on timeout
----------------------

If the request is timed out, ``GRN_CANCEL`` (``-77``)
:doc:`return_code` is returned in response header.

Here is an example response on timeout::

  [
    [
      -77,
      1459846102.63304,
      0.000220775604248047,
      "[request-canceler] a request is canceled: <0x7fa0d5d7ed00>"
    ]
  ]

See :doc:`/reference/command/output_format` for response header.

Enable request timeout by default
---------------------------------

:doc:`/reference/executables/groonga` supports enabling request
timeout by default. You can specify the default request timeout by
:option:`groonga --default-request-timeout`.

If the default request timeout is larger than 0 second, the default
request timeout is used as request timeout for all requests.

You can overwrite the default request timeout by specifying
``request_timeout`` parameter to request. If the default request
timeout is 3 seconds and ``request_timeout`` parameter is 1 second,
the request is canceled after 1 second.

See also
--------

* :doc:`/reference/command/return_code`
* :doc:`/reference/command/output_format`
* :doc:`/reference/commands/request_cancel`
