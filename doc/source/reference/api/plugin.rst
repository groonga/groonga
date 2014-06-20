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

.. c:function:: grn_rc grn_plugin_expr_var_init(grn_ctx *ctx, grn_expr_var *var, const char *name, int name_size);

   It initializes a `grn_expr_var`.

   :param var: The pointer of `grn_expr_var` object to initialize.
   :param name: The name of `grn_expr_var` object to initialize.
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :return: ``GRN_SUCCESS``. It doesn't fail.

.. c:function:: grn_obj * grn_plugin_command_create(grn_ctx *ctx, const char *name, int name_size, grn_proc_func func, unsigned int n_vars, grn_expr_var *vars);

   It creates a command.

   :param name: The `proc` name of the command to create.
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :param func: The function name to be called by the created command.
   :param n_vars: The number of the variables of the command to create.
   :param vars:  The pointer of initialized `grn_expr_var` object.
   :return: ``GRN_SUCCESS``. It doesn't fail.
