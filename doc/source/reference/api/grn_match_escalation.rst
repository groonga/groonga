.. -*- rst -*-

``grn_match_escalation``
========================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:function:: long long int grn_ctx_get_match_escalation_threshold(grn_ctx *ctx)

   検索の挙動をエスカレーションする閾値を返します。エスカレーションの詳細は検索の仕様に関するドキュメントを参照してください。

.. c:function:: grn_rc grn_ctx_set_match_escalation_threshold(grn_ctx *ctx, long long int threshold)

   検索の挙動をエスカレーションする閾値を変更します。エスカレーションの詳細は検索の仕様に関するドキュメントを参照してください。

   :param threshold: 変更後の検索の挙動をエスカレーションする閾値を指定します。

.. c:function:: long long int grn_get_default_match_escalation_threshold(void)

   デフォルトの検索の挙動をエスカレーションする閾値を返します。エスカレーションの詳細は検索の仕様に関するドキュメントを参照してください。

.. c:function:: grn_rc grn_set_default_match_escalation_threshold(long long int threshold)

   デフォルトの検索の挙動をエスカレーションする閾値を変更します。エスカレーションの詳細は詳細は検索の仕様に関するドキュメントを参照してください。

   :param threshold: 変更後のデフォルトの検索の挙動をエスカレーションする閾値を指定します。
