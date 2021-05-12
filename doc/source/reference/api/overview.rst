.. -*- rst -*-

Overview
========

Summary
-------

You can use Groonga as a library. You need to use the following APIs to
initialize and finalize Groonga.

:c:func:`grn_init()` initializes Groonga.
In contrast, :c:func:`grn_fin()` finalizes Groonga.

You must call :c:func:`grn_init()` only once before you use APIs which
are provided by Groonga. You must call :c:func:`grn_fin()` only once
after you finish to use APIs which are provided by Groonga.

Example
-------

Here is an example that uses Groonga as a full-text search library.

.. code-block :: c

   grn_rc rc;
   /* It initializes resources used by Groonga. */
   rc = grn_init();
   if (rc != GRN_SUCCESS) {
     return EXIT_FAILURE;
   }
   /* Some Groonga API calling codes... */
   /* It releases resources used by Groonga. */
   grn_fin();
   return EXIT_SUCCESS;

Reference
---------

.. c:function:: grn_rc grn_init(void)

  ``grn_init()`` initializes resources that are used by Groonga. You
  must call it just once before you call other Groonga APIs.

  :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_fin(void)

  ``grn_fin()`` releases resources that are used by Groonga. You can't
  call other Groonga APIs after you call ``grn_fin()``.

  :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
