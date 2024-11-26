.. -*- rst -*-

``grn_geo``
===========

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_geo_point

.. c:function:: int grn_geo_estimate_in_rectangle(grn_ctx *ctx, grn_obj *index, grn_obj *top_left_point, grn_obj *bottom_right_point)

   It estimates number of records in the rectangle specified by top_left_point parameter and bottom_right_point parameter. Number of records is estimated by index parameter. If an error is occurred, -1 is returned.

   :param index: the index column for TokyoGeoPoint or WGS84GeoPpoint type.
   :param top_left_point: the top left point of the target rectangle. (ShortText, Text, LongText, TokyoGeoPoint or WGS84GeoPoint)
   :param bottom_right_point: the bottom right point of the target rectangle. (ShortText, Text, LongText, TokyoGeoPoint or WGS84GeoPoint)

.. c:function:: grn_obj *grn_geo_cursor_open_in_rectangle(grn_ctx *ctx, grn_obj *index, grn_obj *top_left_point, grn_obj *bottom_right_point, int offset, int limit)

   It opens a cursor to get records in the rectangle specified by top_left_point parameter and bottom_right_point parameter.

   :param index: the index column for TokyoGeoPoint or WGS84GeoPpoint type.
   :param top_left_point: the top left point of the target rectangle. (ShortText, Text, LongText, TokyoGeoPoint or WGS84GeoPoint)
   :param bottom_right_point: the bottom right point of the target rectangle. (ShortText, Text, LongText, TokyoGeoPoint or WGS84GeoPoint)
   :param offset: the cursor returns records from offset parameter position. offset parameter is based on 0.
   :param limit: the cursor returns at most limit parameter records. -1 means no limit.

.. c:function:: grn_posting *grn_geo_cursor_next(grn_ctx *ctx, grn_obj *cursor)

   It returns the next posting that has record ID. It returns NULL after all records are returned.

   :param cursor: the geo cursor.
