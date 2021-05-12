.. -*- rst -*-

``log_level``
=============

Summary
-------

``log_level`` command sets log level of Groonga.

Syntax
------
::

 log_level level

Usage
-----
::

 log_level warning
 [true]

Parameters
----------

``level``

  Specify log level with a character or string which means log level.

  .. list-table::
     :header-rows: 1

     * - Value
       - Alias
     * - ``E``
       - ``emerge`` or ``emergency``
     * - ``A``
       - ``alert``
     * - ``C``
       - ``crit`` or ``critical``
     * - ``e``
       - ``error``
     * - ``w``
       - ``warn`` or ``warning``
     * - ``n``
       - ``notice``
     * - ``i``
       - ``info``
     * - ``d``
       - ``debug``
     * - ``-``
       - ``dump``

  Example::

       emergency
       alert
       critical
       error
       warning
       notice
       info
       debug

Return value
------------

``log_level`` command returns whether log level configuration is succeeded or not::

  [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.

See also
--------

:doc:`log_put`
:doc:`log_reopen`
