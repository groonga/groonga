.. -*- rst -*-

.. highlight:: c

``grn_column``
==============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:function:: grn_rc grn_column_truncate(grn_ctx *ctx, grn_obj *column)

   .. note::

      This is a dangerous API. You must not use this API when other
      thread or process accesses the target column. If you use this
      API against shared column, the process that accesses the column
      may be broken and the column may be broken.

   .. versionadded:: 4.0.9

   Clears all values in the column.

   :param column: The column to be truncated.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
