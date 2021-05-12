.. -*- rst -*-

.. highlight:: c

``grn_content_type``
====================

Summary
-------

:c:type:`grn_content_type` shows input type and output
type. Currently, it is used only for output type.

Normally, you don't need to use this type. It is used internally in
:c:func:`grn_ctx_send()`.

Reference
---------

.. c:type:: grn_content_type

   Here are available values:

   `GRN_CONTENT_NONE`
     It means that outputting nothing or using the original format.
     :doc:`/reference/commands/dump` uses the type.

   `GRN_CONTENT_TSV`
     It means tab separated values format.

   `GRN_CONTENT_JSON`
     It means JSON format.

   `GRN_CONTENT_XML`
     It means XML format.

   `GRN_CONTENT_MSGPACK`
     It means MessagePack format. You need MessagePack library on building
     Groonga. If you don't have MessagePack library, you can't use this type.
