.. -*- rst -*-

``grn_command_version``
=======================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_command_version

.. c:macro:: GRN_COMMAND_VERSION_MIN

.. c:macro:: GRN_COMMAND_VERSION_STABLE

.. c:macro:: GRN_COMMAND_VERSION_MAX

.. c:function:: grn_command_version grn_get_default_command_version(void)

   デフォルトのcommand_versionを返します。

.. c:function:: grn_rc grn_set_default_command_version(grn_command_version version)

   デフォルトのcommand_versionを変更します。

   :param version: 変更後のデフォルトのcommand_versionを指定します。
