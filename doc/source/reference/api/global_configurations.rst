.. -*- rst -*-

Global configurations
=====================

Summary
-------

Groonga has the global configurations. You can access them by API.


Reference
---------

.. note::
   We are currently switching to automatic generation using Doxygen.

.. c:function:: grn_rc grn_set_lock_timeout(int timeout)

   Sets the lock timeout.

   See :c:func:`grn_get_lock_timeout` about lock timeout.

   There are some special values for ``timeout``.

     * ``0``: It means that Groonga doesn't retry acquiring a lock.
       Groonga reports a failure after one lock acquirement failure.
     * negative value: It means that Groonga retries acquiring a lock
       until Groonga can acquire a lock.

   :param timeout: The new lock timeout.
   :return: ``GRN_SUCCESS``. It doesn't fail.
