.. -*- rst -*-

Global configurations
=====================

Summary
-------

Groonga has the global configurations. You can access them by API.


Reference
---------

.. c:function:: int grn_get_lock_timeout(void)

   Returns the lock timeout.

   :c:type:`grn_ctx` acquires a lock for updating a shared value. If
   other :c:type:`grn_ctx` is already updating the same value,
   :c:type:`grn_ctx` that try to acquire a lock can't acquires a lock.
   The :c:type:`grn_ctx` that can't acquires a lock waits 1
   millisecond and try to acquire a lock again. The try is done
   ``timeout`` times. If the :c:type:`grn_ctx` that can't acquires a
   lock until ``timeout`` times, the tries are failed.

   The default lock timeout is ``10000000``. It means that Groonga
   doesn't report a lock failure until about 3 hours.  (1 * 10000000
   [msec] = 10000 [sec] = 166.666... [min] = 2.777... [hour])

   :return: The lock timeout.

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
