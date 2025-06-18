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

.. c:function:: void grn_p_geo_point(grn_ctx *ctx, grn_geo_point *point)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.
   It prints inspected geo point text.

   :param ctx: The context object
   :param point: The inspect target object.

   .. code-block:: c

      grn_obj point;
      int latitude = ((40 * 60 * 60) + (42 * 60) + 46) * 1000;
      int longitude = ((-74 * 60 * 60) + (0 * 60) + 22) * 1000;
      GRN_WGS84_GEO_POINT_INIT(&point, 0);
      GRN_GEO_POINT_SET(&context, &point, latitude, longitude);
      grn_p_geo_point(&context, (grn_geo_point*)&point);

   If ``point`` indicates New York City, it prints like the following::

     [(524290,18) ((0, 8, 44, 290),(0, 0, 0, 18)) [00000000 00000000 00000000 10000000 00000000 00000000 00000001 00001100]]

.. c:function:: void grn_p_ii_values(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.
   It prints inspected index values.

   :param ctx: The context object
   :param obj: The inspect target object.

   .. code-block:: c

      grn_p_ii_values(&context, obj);

   If ``obj`` is an index column, it prints like the following::

     [
       #<"!"
         elements:[
           {status:available, rid:1, sid:1, pos:0, tf:1, weight:0, rest:1},
           {status:available, rid:2, sid:1, pos:0, tf:1, weight:0, rest:1}
         ]
       >,
       ...
