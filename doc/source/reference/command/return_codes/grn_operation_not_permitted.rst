.. -*- rst -*-

``-2: GRN_OPERATION_NOT_PERMITTED``
===================================

Major cause
-----------

1. If the table that is a target of ``table_remove`` has reference from other table and column.
2. If the number of threads is greater than 1 when we execute ``database_unmap``.
3. (Windows only) If network access is denied.

Major action on this error
--------------------------

1. If the table that is a target of ``table_remove`` has reference from other table and column.

   We remove tables and columns of the reference sources.

2. If the number of threads is greater than 1 when we execute ``database_unmap``.

   We make the number of threads one by executing ``thread_limit max=1``.

3. (Windows only) If network access is denied.

   We confirm the status network(e.g. confirm whether the server process starting, confirm the setting of the firewall, and so on).
