.. -*- rst -*-

``grn_ii``
==========

Summary
-------

buffered index builder

特定のアプリケーション用に準備した内部APIです。

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_ii

.. c:type:: grn_ii_buffer

.. c:function:: grn_ii_buffer *grn_ii_buffer_open(grn_ctx *ctx, grn_ii *ii, unsigned long long int update_buffer_size)

.. c:function:: grn_rc grn_ii_buffer_append(grn_ctx *ctx, grn_ii_buffer *ii_buffer, grn_id rid, unsigned int section, grn_obj *value)

.. c:function:: grn_rc grn_ii_buffer_commit(grn_ctx *ctx, grn_ii_buffer *ii_buffer)

.. c:function:: grn_rc grn_ii_buffer_close(grn_ctx *ctx, grn_ii_buffer *ii_buffer)
