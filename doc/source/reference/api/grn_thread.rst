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

.. note::
   We are currently switching to automatic generation using Doxygen.
