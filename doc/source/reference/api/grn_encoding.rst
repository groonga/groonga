.. -*- rst -*-

``grn_encoding``
================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_encoding

   TODO...

.. c:function:: const char *grn_encoding_to_string(grn_encoding encoding)

   Returns string representation for the encoding. For example, 'grn_encoding_to_string(``GRN_ENC_UTF8``)' returns '"utf8"'.
 
   "unknown" is returned for invalid encoding.

   :param encoding: The encoding.

.. c:function:: grn_encoding grn_encoding_parse(const char *name)
 
   Parses encoding name and returns grn_encoding. For example, 'grn_encoding_parse("UTF8")' returns '``GRN_ENC_UTF8``'.
 
   ``GRN_ENC_UTF8`` is returned for invalid encoding name.

   :param name: The encoding name.
