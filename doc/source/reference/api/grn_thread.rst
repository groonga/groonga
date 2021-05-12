.. -*- rst -*-

``grn_thread_*``
================

Summary
-------

Groonga provides thread related APIs with ``grn_thread_`` prefix.

Normally, you don't need to use these APIs.

You may want to use these APIs when you write a Groonga server.

Example
-------

Here is a real word use case of ``grn_thread_*`` APIs by
:doc:`/reference/executables/groonga`. :doc:`/reference/executables/groonga`
increases its thread pool size when the max number of threads is
increased. :doc:`/reference/executables/groonga` decreases its thread
pool size and stops too many threads when the max number of threads is
decreased.

.. code-block :: c

   static grn_mutex q_mutex;
   static grn_cond q_cond;
   static uint32_t nfthreads;
   static uint32_t max_nfthreads;

   static uint32_t
   groonga_get_thread_limit(void *data)
   {
     return max_nfthreads;
   }

   static void
   groonga_set_thread_limit(uint32_t new_limit, void *data)
   {
     uint32_t i;
     uint32_t current_nfthreads;

     MUTEX_LOCK(q_mutex);
     current_nfthreads = nfthreads;
     max_nfthreads = new_limit;
     MUTEX_UNLOCK(q_mutex);

     if (current_nfthreads > new_limit) {
       for (i = 0; i < current_nfthreads; i++) {
         MUTEX_LOCK(q_mutex);
         COND_SIGNAL(q_cond);
         MUTEX_UNLOCK(q_mutex);
       }
     }
   }

   int
   main(int argc, char *argv)
   {
     /* ... */
     grn_thread_set_get_limit_func(groonga_get_thread_limit, NULL);
     grn_thread_set_set_limit_func(groonga_set_thread_limit, NULL);

     grn_init();

     /* ... */
   }

Reference
---------

.. c:type:: uint32_t (*grn_thread_get_limit_func)(void *data)

   It's the type of function that returns the max number of threads.

.. c:type:: void (*grn_thread_set_limit_func)(uint32_t new_limit, void *data)

   It's the type of function that sets the max number of threads.

.. c:function:: uint32_t grn_thread_get_limit(void)

   It returns the max number of threads.

   If :c:type:`grn_thread_get_limit_func` isn't set by
   :c:func:`grn_thread_set_get_limit_func()`, it always returns ``0``.

   :return: The max number of threads or ``0``.

.. c:function:: void_t grn_thread_set_limit(uint32_t new_limit)

   It sets the max number of threads.

   If :c:type:`grn_thread_set_limit_func` isn't set by
   :c:func:`grn_thread_set_set_limit_func()`, it does nothing.

   :param new_limit: The new max number of threads.

.. c:function:: void grn_thread_set_get_limit_func(grn_thread_get_limit_func func, void *data)

   It sets the custom function that returns the max number of threads.

   ``data`` is passed to ``func`` when ``func`` is called from
   :c:func:`grn_thread_get_limit()`.

   :param func: The custom function that returns the max number of threads.
   :param data: An user data to be passed to ``func`` when ``func``
                is called.

.. c:function:: void grn_thread_set_set_limit_func(grn_thread_set_limit_func func, void *data)

   It sets the custom function that sets the max number of threads.

   ``data`` is passed to ``func`` when ``func`` is called from
   :c:func:`grn_thread_set_limit()`.

   :param func: The custom function that sets the max number of threads.
   :param data: An user data to be passed to ``func`` when ``func``
                is called.

