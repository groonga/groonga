.. -*- rst -*-

``-34: GRN_RESOURCE_DEADLOCK_AVOIDED``
======================================

Major cause
-----------

Groonga gets a lock to block write to object when we execute ``load`` or ``delete``, and so on.
At that time, if Groonga has been already got the lock, Groonga wait to release the loack.
This error occurs when Groonga can't get the lock while about 3 hours.

Major action on this error
--------------------------

If this error occurs, there is a high probability of that Groonga's data or indexes are corrupt.

We confirm whether Groonga's database is corrupt or not.

1. We stop Groonga. (This is a very important procedure. We must execute this procedure.)

2. We execute ``grndb check DATABASE_PATH`` against Groonga's database.

   Please see ::doc::`/reference/executables/grndb.html` about ``grndb``.

3. We confirm the log of ``grndb check`` to whether Groonga's database can recover or not.

4. If Groonga's database can recover, we execute ``grndb recover DATABASE_PATH``.

   If Groonga's database can not recover, we restore Groonga's database from backup.
