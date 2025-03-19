.. -*- rst -*-

.. highlight:: c

``grn_cache``
=============

Summary
-------

.. note::

   This API is experimental.

``grn_cache`` is a data store that keeps responses of
:doc:`/reference/commands/select` command. It is not general use cache
object. It is only for :doc:`/reference/commands/select` command.

You can just change the current cache object by
:c:func:`grn_cache_current_set()`. :doc:`/reference/commands/select`
command response cache is done internally.

:doc:`/reference/commands/select` command uses one global cache
object. If you open multiple databases, the one cache is shared. It is
an important problem.

If you open multiple databases and use
:doc:`/reference/commands/select` command, you need to use
``grn_cache`` object. It is :doc:`/reference/executables/groonga-httpd`
case. If you open only one database or don't use
:doc:`/reference/commands/select` command, you don't need to use
``grn_cache`` object. It is `rroonga
<http://ranguba.org/#about-rroonga>`_ case.

Example
-------

Here is an example that change the current cache object.

.. code-block:: c

   grn_cache *cache;
   grn_cache *cache_previous;
   cache = grn_cache_open(ctx);
   cache_previous = grn_cache_current_get(ctx);
   grn_cache_current_set(ctx, cache);
   /* grn_ctx_send(ctx, ...); */
   grn_cache_current_set(ctx, cache_previous);


Reference
---------

.. c:type:: grn_cache

   It is an opaque cache object. You can create a ``grn_cache`` by
   :c:func:`grn_cache_open()` and free the created object by
   :c:func:`grn_cache_close()`.
