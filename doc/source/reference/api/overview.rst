.. -*- rst -*-

.. highlightlang:: none

Overview
========

Summary
-------

Groonga can use as library. You can start to use Groonga as full-text search
library with the following API.

``grn_init`` is initializer for libgroonga.
In contrast, ``grn_fin`` is finalizer for libgroonga.

You must call ``grn_init`` once before using API which are provided by libgroonga.
You must call ``grn_fin`` once after stop to use API which are provided by libgroonga.

Example
-------

Here is an example that uses Groonga as full-text search library.

.. code-block :: c

   grn_rc rc;
   /* Preparing resource will be used by libgroonga. */
   rc = grn_init();
   if (rc != GRN_SUCCESS) {
     return EXIT_FAILURE;
   }
   /* Some Groonga API calling codes... */
   /* Releasing resource used by libgroonga. */
   grn_fin();
   return EXIT_SUCCESS;

Reference
---------

.. c:function:: grn_rc grn_init(void)

  `grn_init()` initializes resource that is used by libgroonga. You must call it once before calling other Groonga API.

  :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_fin(void)

  `grn_fin()` releases resource that is used by libgroonga. You must not call it other Groonga API after calling `grn_fin()`.

  :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
