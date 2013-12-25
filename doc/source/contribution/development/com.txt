.. -*- rst -*-

Groonga 通信アーキテクチャ
==========================

GQTPでのアーキテクチャ
----------------------

- comが外部からの接続を受け付ける。
- comは1スレッド。
- comがedgeを作る。
- edgeは接続と１対１対応。
- edgeはctxを含む。
- workerはthreadと１対１対応。
- workerは上限が個定数。
- workerは、１つのedgeと結びつくことができる。

- edgeごとにqueueを持つ。
- msgはcomによって、edgeのqueueにenqueueされる。
  edgeがworkerに結びついていないときは、同時に、ctx_newというqueueに、msgをenqueueした対象のedgeをenqueueする。
