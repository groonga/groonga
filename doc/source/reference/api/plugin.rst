.. -*- rst -*-

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

.. c:function:: grn_rc GRN_PLUGIN_INIT(grn_ctx *ctx)

.. c:function:: grn_rc GRN_PLUGIN_REGISTER(grn_ctx *ctx)

.. c:function:: grn_rc GRN_PLUGIN_FIN(grn_ctx *ctx)

.. c:macro:: GRN_PLUGIN_MALLOC(ctx, size)

   GRN_PLUGIN_MALLOC() allocates `size` bytes and returns a pointer to the
   allocated memory space. Note that the memory space is associated with `ctx`.

.. c:macro:: GRN_PLUGIN_REALLOC(ctx, ptr, size)

   GRN_PLUGIN_REALLOC() resizes the memory space pointed to by `ptr` or
   allocates a new memory space of `size` bytes. GRN_PLUGIN_REALLOC() returns
   a pointer to the memory space. The contents is unchanged or copied from the
   old memory space to the new memory space.

.. c:macro:: GRN_PLUGIN_FREE(ctx, ptr)

   GRN_PLUGIN_FREE() frees a memory space allocated by GRN_PLUGIN_MALLOC() or
   GRN_PLUGIN_REALLOC(). This means that `ptr` must be a pointer returned by
   GRN_PLUGIN_MALLOC() or GRN_PLUGIN_REALLOC().

.. c:macro:: GRN_PLUGIN_LOG(ctx, level, ...)

   GRN_PLUGIN_LOG() reports a log of `level`. Its error message is generated
   from the varying number of arguments, in which the first one is the format
   string and the rest are its arguments. See grn_log_level in "groonga.h" for
   more details of `level`.

.. c:macro:: GRN_PLUGIN_ERROR(ctx, error_code, ...)

   GRN_PLUGIN_ERROR() reports an error of `error_code`. Its error message is
   generated from the varying number of arguments, in which the first one is the
   format string and the rest are its arguments. See grn_rc in "groonga.h" for
   more details of `error_code`.

.. c:type:: grn_plugin_mutex

   grn_plugin_mutex is available to make a critical section. See the
   following functions.

.. c:function:: grn_plugin_mutex *grn_plugin_mutex_open(grn_ctx *ctx)

   grn_plugin_mutex_open() returns a pointer to a new object of
   grn_plugin_mutex. Memory for the new object is obtained with
   GRN_PLUGIN_MALLOC(). grn_plugin_mutex_open() returns NULL if sufficient
   memory is not available.

.. c:function:: void grn_plugin_mutex_close(grn_ctx *ctx, grn_plugin_mutex *mutex)

   grn_plugin_mutex_close() finalizes an object of grn_plugin_mutex and then
   frees memory allocated for that object.

.. c:function:: void grn_plugin_mutex_lock(grn_ctx *ctx, grn_plugin_mutex *mutex)

   grn_plugin_mutex_lock() locks a mutex object. If the object is already
   locked, the calling thread waits until the object will be unlocked.

.. c:function:: void grn_plugin_mutex_unlock(grn_ctx *ctx, grn_plugin_mutex *mutex)

   grn_plugin_mutex_unlock() unlocks a mutex object. grn_plugin_mutex_unlock()
   should not be called for an unlocked object.

.. c:function:: grn_obj *grn_plugin_proc_alloc(grn_ctx *ctx, grn_user_data *user_data, grn_id domain, grn_obj_flags flags)

   grn_plugin_proc_alloc() allocates a `grn_obj` object.
   You can use it in function that is registered as GRN_PROC_FUNCTION.

.. c:function:: grn_obj grn_plugin_proc_get_var(grn_ctx *ctx, grn_user_data *user_data, const char *name, int name_size)

   It gets a variable value from `grn_user_data` by specifying the variable name.

   :param name: The variable name.
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :return: A variable value on success, NULL otherwise.

.. c:function:: grn_obj *grn_plugin_proc_get_var_by_offset(grn_ctx *ctx, grn_user_data *user_data, unsigned int offset)

   It gets a variable value from `grn_user_data` by specifying the offset position of the variable.

   :param offset: The offset position of the variable.
   :return: A variable value on success, NULL otherwise.

.. c:function:: const char *grn_plugin_win32_base_dir(void)

   .. deprecated:: 5.0.9. Use :c:func:`grn_plugin_windows_base_dir()`
                   instead.

   It returns the Groonga install directory. The install directory is
   computed from the directory that has ``groonga.dll``. You can use
   the directory to generate install directory aware path. It only
   works on Windows. It returns ``NULL`` on other platforms.

.. c:function:: const char *grn_plugin_windows_base_dir(void)

   .. versionadded:: 5.0.9

   It returns the Groonga install directory. The install directory is
   computed from the directory that has ``groonga.dll``. You can use
   the directory to generate install directory aware path. It only
   works on Windows. It returns ``NULL`` on other platforms.

.. c:function:: int grn_plugin_charlen(grn_ctx *ctx, const char *str_ptr, unsigned int str_length, grn_encoding encoding)

   grn_plugin_charlen() returns the length (#bytes) of the first character
   in the string specified by `str_ptr` and `str_length`. If the starting bytes
   are invalid as a character, grn_plugin_charlen() returns 0. See
   grn_encoding in "groonga.h" for more details of `encoding`.

.. c:function:: int grn_plugin_isspace(grn_ctx *ctx, const char *str_ptr, unsigned int str_length, grn_encoding encoding)

   grn_plugin_isspace() returns the length (#bytes) of the first character
   in the string specified by `str_ptr` and `str_length` if it is a space
   character. Otherwise, grn_plugin_isspace() returns 0.

.. c:function:: grn_rc grn_plugin_expr_var_init(grn_ctx *ctx, grn_expr_var *var, const char *name, int name_size)

   It initializes a `grn_expr_var`.

   :param var: The pointer of `grn_expr_var` object to be initialized.
   :param name: The name of `grn_expr_var` object to be initialized.
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :return: ``GRN_SUCCESS``. It doesn't fail.

.. c:function:: grn_obj * grn_plugin_command_create(grn_ctx *ctx, const char *name, int name_size, grn_proc_func func, unsigned int n_vars, grn_expr_var *vars)

   It creates a command.

   :param name: The `proc` name of the command to be created.
   :param name_size: The number of bytes of name. If `name_size` is negative, `name` must be NUL-terminated. `name_size` is computed by `strlen(name)` for the case.
   :param func: The function name to be called by the created command.
   :param n_vars: The number of the variables of the command to create.
   :param vars:  The pointer of initialized `grn_expr_var` object.
   :return: The created command object if it creates a command successfully,
            `NULL` otherwise. See `ctx` for error details.
