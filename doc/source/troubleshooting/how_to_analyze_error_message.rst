.. -*- rst -*-

.. highlightlang:: none

エラーメッセージの解析方法
==========================

Groongaは様々なエラーメッセージを出力しますが、出力されたエラーメッセージ
をもとに、原因を解析する方法はいままで、明文化していませんでした。
ここでは、エラーメッセージごとの解析方法を記載します。


ソケットエラーの解析方法
------------------------

ここでは、Groongaで発生するソケットエラーの解析方法について説明します。


例
^^

Groongaのエラーログで以下のようなエラーログがあります。(xxxxxには任意の数字が入ります。)::

  socket error[xxxxx]: no buffer: accept


解析方法
^^^^^^^^

まず、ソケットエラーを扱うマクロである、SOERR というキーワードでGroongaのソースコードをgrepします。

次に見つかったSOERRの引数にacceptが入っているSOERRをさがします。
すると次のSOERRが見つかります。ログに出ているのはacceptのみなので、下記の最後の行が例で出力されたエラーメッセージに該当するとわかります。::

  lib/com.c:      SOERR("listen - start accept");
  lib/com.c:      SOERR("listen - disable accept");
  lib/com.c:        SOERR("accept");


該当するエラー出力の周辺のコードを見ると以下のようになっています。::

  grn_sock fd = accept(com->fd, NULL, NULL);
  if (fd == -1) {
    if (errno == EMFILE) {
      grn_com_event_stop_accept(ctx, ev);
    } else {
      SOERR("accept");
    }
  return;
  }

上記のコードから、acceptを実行してエラーが発生したことが確認できます。
次は、acceptが失敗した原因を追っていきます。

acceptが失敗した理由は、::

  [10055]: no buffer

から追うことができます。
10055はWindowsのソケットエラーコードを表しています。また、no bufferはSOERRマクロ内でGroongaが用意しているメッセージです。
Windowsのシステムエラーコードから調査しても良いですし、Groongaが出力しているエラーメッセージから調査しても良いです。

Windowsのシステムエラーコードは以下のページに一覧があります。::

  https://msdn.microsoft.com/ja-jp/library/windows/desktop/ms740668(v=vs.85).aspx

システムエラーコードまたは、エラーメッセージから調査すると、acceptが失敗した理由は、WSAENOBUFSであることがわかります。さらに、WSAENOBUFSが発生する原因を調査するとWSAENOBUFSが発生する原因は以下であることがわかります。::

  No buffer space available.

  An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.

上記のメッセージから、acceptが失敗する原因は、メモリー不足または、接続数が多すぎる場合であることがわかりました。
あとは、エラーメッセージが出た際の状況から、接続数が多かったのか、メモリ不足だったのかを判断します。
