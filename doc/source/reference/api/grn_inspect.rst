.. -*- rst -*-

``grn_inspect``
===============

Summary
-------

There are two kind of functions to inspect :c:type:`grn_obj`. One is ``grn_inspect`` function, The other is ``grn_p`` function.

Here is the list of ``grn_inspect`` function series. It sets inspected text into specified object.

* :c:func:`grn_inspect()`
* :c:func:`grn_inspect_indented()`
* :c:func:`grn_inspect_limited()`
* :c:func:`grn_inspect_name()`
* :c:func:`grn_inspect_encoding()`
* :c:func:`grn_inspect_type()`
* :c:func:`grn_inspect_query_log_flags()`

Here is the list of ``grn_p`` function series. It prints inspected text into console.

* :c:func:`grn_p()`
* :c:func:`grn_p_geo_point()`
* :c:func:`grn_p_ii_values()`

Example
-------

Here is an example which inspects specified target object.

.. code-block:: c

   grn_obj buffer;
   GRN_TEXT_INIT(&buffer, 0);
   grn_inspect(&context, &buffer, obj);
   /* equivalent to grn_p(ctx, obj); */
   printf("inspected: <%.*s>\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

Reference
---------

.. note::
   We are currently switching to automatic generation using Doxygen.
