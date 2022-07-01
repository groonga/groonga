.. -*- rst -*-

``-3: GRN_NO_SUCH_FILE_OR_DIRECTORY``
=====================================

Major cause
-----------

1. If we registered the plugin that we didn't install yet with ``plugin_register``.

   For example, if we executed ``plugin_register normalizers/mysql`` on Groonga when we didn't install `` groonga-normalizer-mysql`` yet.

2. If we used ``TokenMecab`` when ``mecab`` didn't install yet.

Major action on this error
--------------------------

1. If we registered the plugin that we didn't install yet with ``plugin_register``.

   We install the target plugin.

2. If we used ``TokenMecab`` when ``mecab`` didn't install yet.

   We install ``mecab``.
