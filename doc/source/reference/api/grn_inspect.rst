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

.. c:function:: grn_obj *grn_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.

   .. note:: A table is specified and it's table type is ``TABLE_PAT_KEY``, all keys are shown.
             If you do not want to this behavior, use :c:func:`grn_inspect_limited` instead.

   :param ctx: The context object
   :param buffer: The buffer object which is inspected text will be stored.
   :param obj: The inspect target object.
   :return: ``buffer`` object which is inspected text is set.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect(&context, &buffer, obj);
      printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   If obj is ``TABLE_PAT_KEY`` table, it prints like the following::

     #<table:pat Users key:ShortText value:(nil) size:7 columns:[] default_tokenizer:(nil) normalizer:(nil) keys:["a", "b", "c", "d", "e", "f", "g"] subrec:none nodes:{
     4{0,5,0}
       L:2{0,6,0}
         L:1{0,7,0}
           L:0{0,0,0}
           R:1{0,7,0}("a")[01100001]
     ...

.. c:function:: grn_obj *grn_inspect_indented(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj, const char *indent)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.

   .. note:: ``indent`` is only added if inspected text contains newline (inspected text must be multiple lines).

   :param ctx: The context object
   :param buffer: The buffer object which is inspected text will be stored.
   :param obj: The inspect target object.
   :param indent: The pre-pended indentation text.
   :return: ``buffer`` object which is inspected text is set with indent.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect_indented(&context, &buffer, obj, "***");
      printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   If obj is ``TABLE_PAT_KEY`` table, it prints like the following::

     ***#<table:pat Users key:ShortText value:(nil) size:7 columns:[] default_tokenizer:(nil) normalizer:(nil) keys:["a", "b", "c", "d", "e", "f", "g"] subrec:none nodes:{
     ***4{0,5,0}
     ***  L:2{0,6,0}
     ***    L:1{0,7,0}
     ***      L:0{0,0,0}
     ***      R:1{0,7,0}("a")[01100001]
     ...

.. c:function:: grn_obj *grn_inspect_limited(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj)

   .. versionadded:: 7.0.0

   Inspect specified target ``obj`` object.

   .. note:: If inspected text is too long, it will be truncated.

   :param ctx: The context object
   :param buffer: The buffer object which is inspected(truncated) text will be stored.
   :param obj: The inspect target object.
   :return: ``buffer`` object which is object detail is set.
            If inspected text is longer than 64 characters, inspected text is truncated to it. Otherwise, inspected text will not be truncated.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect(&context, &buffer, obj);
      printf("#=> %.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   Even though if obj is ``TABLE_PAT_KEY`` table, it prints truncated result like the following::

     #<table:pat Users key:ShortText value:(nil) size:7 columns:[] de...(502)

.. c:function:: grn_obj *grn_inspect_name(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.

   :param ctx: The context object
   :param buffer: The buffer object which is object name will be stored.
   :param obj: The inspect target object.
   :return: ``buffer`` object which is name of object is set.
            If target object is nil, ``(nil)`` is set to buffer, if target object is internally used object, ``(anonymouse: ID)`` is set to buffer.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect_name(&context, &buffer, obj);
      printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   Specified object name is printed like this::

     Users

.. c:function:: grn_obj *grn_inspect_encoding(grn_ctx *ctx, grn_obj *buffer, grn_encoding encoding)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.

   :param ctx: The context object
   :param buffer: The buffer object which is encoding name will be stored.
   :param encoding: The inspect target encoding. encoding must be ``GRN_ENC_DEFAULT``, ``GRN_ENC_NONE``, ``GRN_ENC_EUC_JP``, ``GRN_ENC_UTF8``, ``GRN_ENC_SJIS``, ``GRN_ENC_LATIN1`` or ``GRN_ENC_KOI8R``
   :return: ``buffer`` object which is encoding name is set.
            If invalid ``encoding`` is given, ``(unknown: ENCODING)`` is set to ``buffer``.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect_encoding(&context, &buffer, GRN_ENC_UTF8);
      printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   Specified encoding name is printed like the following::

     UTF-8

.. c:function:: grn_obj *grn_inspect_type(grn_ctx *ctx, grn_obj *buffer, unsigned char type)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.

   :param ctx: The context object
   :param buffer: The buffer object which is type name will be stored.
   :param type: The inspect target type.
   :return: ``buffer`` object which is type name is set.
            If invalid ``type`` is given, ``(unknown: TYPE_IN_HEX)`` is set to ``buffer``.

   .. code-block:: c

      grn_obj buffer;
      GRN_TEXT_INIT(&buffer, 0);
      grn_inspect_type(&context, &buffer, obj->header.type);
      printf("#=> %.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   If obj is builtin type, type name is printed like the following::

     GRN_TYPE

.. c:function:: grn_obj *grn_inspect_query_log_flags(grn_ctx *ctx, grn_obj *buffer, unsigned int flags)

   .. versionadded:: 7.0.4

   Inspect specified target ``flag``.

   :param ctx: The context object
   :param buffer: The buffer object which is flag name will be stored.
   :param flags: The inspect target type.
   :return: ``buffer`` object which is flag name is set.
            If invalid ``flags`` is given, empty string is set to ``buffer``.

   .. code-block:: c

       grn_obj buffer;
       GRN_TEXT_INIT(&buffer, 0);
       int current_flags = grn_query_logger_get_flags(&context);
       grn_inspect_query_log_flags(&context, &buffer, current_flags);
       printf("%.*s\n", (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));

   The query logger flags are printed like the following::

     COMMAND|RESULT_CODE|DESTINATION|CACHE|SIZE|SCORE

.. c:function:: void grn_p(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 4.0.8

   Inspect specified target ``obj`` object.
   It prints inspected text.

   :param ctx: The context object
   :param obj: The inspect target object.

   .. code-block:: c

      grn_p(&context, &buffer, obj);

   If obj is ``ShortText``, it prints like the following::

     #<type ShortText size:4096 type:var_size>

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
