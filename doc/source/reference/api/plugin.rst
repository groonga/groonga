.. -*- rst -*-

.. highlightlang:: none

Plugin
======

Summary
-------

Groonga supports plugin. You can create a new plugin with the
following API.

TOOD: Describe about how to create the minimum plugin here or create a
tutorial about it.

Reference
---------

.. c:function:: grn_obj grn_plugin_proc_get_var(grn_ctx *ctx, grn_user_data *user_data, const char *name, int name_size)

   It gets a variable value from `grn_user_data` by specify the variable name.

   :param name: The variable name
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :return: A variable value on success, NULL otherwise.

.. c:function:: grn_obj *grn_plugin_proc_get_var_by_offset(grn_ctx *ctx, grn_user_data *user_data, unsigned int offset);

   It gets a variable value from `grn_user_data` by specify the offset position of the variable.

   :param offset: The offset position of the variable.
   :return: A variable value on success, NULL otherwise.

