.. -*- rst -*-

リリース手順
============

前提条件
*********

リリース手順の前提条件は以下の通りです。

* ビルド環境は Debian GNU/Linux (sid)
* コマンドラインの実行例はzsh

作業ディレクトリ例は以下を使用します。

* GROONGA_DIR=$HOME/work/groonga
* GROONGA_CLONE_DIR=$HOME/work/groonga/groonga.clean
* GROONGA_ORG_PATH=$HOME/work/groonga/groonga.org
* APACHE_ARROW_REPOSITORY=$HOME/work/apache/arrow

最初の1回だけ行う手順
**********************

ビルド環境の準備
----------------

以下にGroongaのリリース作業を行うために事前にインストール
しておくべきパッケージを示します。

なお、ビルド環境としては Debian GNU/Linux (sid)を前提として説明しているため、その他の環境では適宜読み替えて下さい。

.. code-block:: console

   $ sudo apt-get install -V debootstrap createrepo rpm mercurial python-docutils python-jinja2 ruby-full mingw-w64 g++-mingw-w64 mecab libmecab-dev nsis gnupg2 dh-autoreconf bison

また、rubyのrakeパッケージを以下のコマンドによりインストールします。

.. code-block:: console

   $ sudo gem install rake

パッケージ署名用秘密鍵のインポート
----------------------------------

リリース作業ではRPMパッケージに対する署名を行います。
その際、パッケージ署名用の鍵が必要です。

Groongaプロジェクトでは署名用の鍵をリリース担当者の公開鍵で暗号化してリポジトリのpackages/ディレクトリ以下へと登録しています。新しいリリース担当者に任命されたばかりで、まだ自分用に暗号化された鍵が無い場合には、他のリリース担当者に依頼して署名用の鍵を暗号化してもらって下さい。

リリース担当者はリポジトリに登録された秘密鍵を復号した後に鍵のインポートを以下のコマンドにて行います。

.. code-block:: console

   $ cd packages
   $ gpg --decrypt release-key-secret.asc.gpg.(担当者) > (復号した鍵ファイル)
   $ gpg --import  (復号した鍵ファイル)

鍵のインポートが正常終了すると gpg --list-keys でGroongaの署名用の鍵を確認することができます。

.. code-block:: console

   $ gpg --list-keys
   pub   1024R/F10399C0 2012-04-24
   uid                  groonga Key (groonga Official Signing Key)
   <packages@groonga.org>
   sub   1024R/BC009774 2012-04-24

鍵をインポートしただけでは使用することができないため、インポートした鍵に対してtrust,signを行う必要があります。

以下のコマンドを実行して署名を行います。(途中の選択肢は省略)

.. code-block:: console

   $ gpg --edit-key packages@groonga.org
   gpg> trust
   gpg> sign
   gpg> save
   gpg> quit

この作業は、新規にリリースを行うことになった担当者やパッケージに署名する鍵に変更があった場合などに行います。

PPA用の鍵の登録
---------------

この作業は、新規にリリースを行うことになった担当者のみ行います。

[Launchpad](https://launchpad.net/)にアカウントを作成し、自分の普段使いの公開鍵を登録した上で、他のリリース担当者に依頼して[Groongaチーム](https://launchpad.net/~groonga)に追加してもらって下さい。


リリース作業用ディレクトリの作成
--------------------------------

Groongaのリリース作業ではリリース専用の環境下(コンパイルフラグ)でビルドする必要があります。

リリース時と開発時でディレクトリを分けずに作業することもできますが、誤ったコンパイルフラグでリリースしてしまう危険があります。

そのため、以降の説明では$GROONGA_DIR以下のディレクトリにリリース用の作業ディレクトリ(groonga.clean)としてソースコードをcloneしたものとして説明します。

毎回のリリースで行う手順
************************

Groongaのソースコード取得
-------------------------

リリース用のクリーンな状態でソースコードを取得するために$GROONGA_DIRにて以下のコマンドを実行します。

.. code-block:: console

   $ git clone --recursive git@github.com:groonga/groonga.git groonga.clean

この作業はリリース作業ごとに行います。

Groongaのウェブサイトの取得
---------------------------

GroongaのウェブサイトのソースはGroonga同様にGitHubにリポジトリを置いています。

リリース作業では後述するコマンド( ``rake release:version:update`` )でタグをプッシュしてCIが動き出すトリガーとします。

.. code-block:: console

   $ git clone git@github.com:groonga/groonga.org.git ${GROONGA_ORG_PATH}

これで、$GROONGA_ORG_PATHにgroonga.orgのソースを取得できます。

Apache Arrowリポジトリの取得
----------------------------

Apache Arrowのウェブサイトのソースも取得します。Apache ArrowのRakeタスクを利用するためです。

.. code-block:: console

   $ git clone https://github.com/apache/arrow.git ${APACHE_ARROW_REPOSITORY}

事前確認
--------

Ubuntu向けパッケージをテスト用に公開する時は、 :ref:`build-for-ubuntu-nightly` の手順で不安定版のリポジトリにアップロードするように指定します。

新任のリリース担当者は必ず、この方法でPPAのリポジトリにパッケージをアップロードできる事を確認しておいてください。

PPAのリポジトリは、同名のパッケージを上書いてアップロードできないので、不安定版のリポジトリでビルドできることを確認してから、安定版のリポジトリへアップロードするようにしてください。

変更点のまとめ
--------------

前回リリース時からの変更点を ``$GROONGA_CLONE_DIR/doc/source/news/*.md`` （英語）にまとめます。
ここでまとめた内容についてはリリースアナウンスにも使用します。

前回リリースからの変更履歴を参照するには以下のコマンドを実行します。

.. code-block:: console

   $ git log -p --reverse $(git tag --sort=taggerdate | tail -1)..

ログを^commitで検索しながら、以下の基準を目安として変更点を追記していきます。

含めるもの

* ユーザへ影響するような変更
* 互換性がなくなるような変更

含めないもの

* 内部的な変更(変数名の変更やらリファクタリング)

``rake release`` の実行
-----------------------

``rake release`` コマンドでは、 ``NEW_RELEASE_DATE`` にリリースの日付（≒ 実行日）を指定します。

.. code-block:: console

   $ cd ${GROONGA_CLONE_DIR}
   $ rake release NEW_RELEASE_DATE=$(date +%Y-%m-%d)

``release`` タスクは次の3つのタスクを実行します。

1. ``release:version:update``

   - RPMパッケージのspecファイルのバージョン表記などを更新します。

2. ``release:tag``

   - リリース用のタグを打ちます。

   - これによりタグがプッシュされ自動リリースが動き出します。

3. ``dev:version:bump``

   - 次のリリースに向けてバージョンを更新します。

補足: ``dev:version:bump`` タスク
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``rake dev:version:bump NEW_VERSION=x.x.x`` のようにバージョンを指定して更新できます。

.. note::
   base_versionはtar.gzなどのリリース用のファイル名で使用します。

.. _build-for-ubuntu-nightly:

Ubuntu向けパッケージのビルド確認
--------------------------------

Ubuntu向けのパッケージは、Launchpadでビルドしています。
リリース前にUbuntu向けパッケージが正常にビルドできるか以下の手順で確認します。

``rake release:version:update`` の結果をリポジトリーにpush後にGitHub Actionsで生成されるソースアーカイブをダウンロードします。
ダウンロードしたソースアーカイブを ``$GROONGA_CLONE_DIR`` のトップに配置します。その後、以下のコマンドを実行してください。

.. code-block:: console

   $ cd $GROONGA_CLONE_DIR/packages
   $ rake ubuntu DPUT_CONFIGURATION_NAME=groonga-ppa-nightly DPUT_INCOMING="~groonga/ubuntu/nightly" LAUNCHPAD_UPLOADER_PGP_KEY=xxxxxxx

各種テストの確認
----------------

リリース用のタグを設定する前に、以下のテストが全てパスしているかを確認します。
タグを設定してから問題が発覚すると、再度リリースすることになってしまうので、タグを設定する前に問題がないか確認します。

* `GitHub Actions <https://github.com/groonga/groonga/actions?query=workflow%3APackage>`_
* `Launchpad <https://launchpad.net/~groonga/+archive/ubuntu/nightly/+packages>`_

テストやパッケージの作成に失敗していたら、原因を特定して修正します。

Ubuntu用パッケージのアップロード
--------------------------------

Ubuntu向けパッケージの作成には、作業マシン上にGroongaのビルドに必要な依存ソフトウェア一式がインストールされている必要があります。以下のようにしてインストールしておいて下さい。

.. code-block:: console

   $ sudo apt build-dep groonga

Ubuntu向けのパッケージのアップロードには以下のコマンドを実行します。

.. code-block:: console

   $ cd packages
   $ rake ubuntu LAUNCHPAD_UPLOADER_PGP_KEY=xxxxxxx

アップロードが正常終了すると、launchpad.net上でビルドが実行され、ビルド結果がメールで通知されます。ビルドに成功すると、リリース対象のパッケージがlaunchpad.netのGroongaチームのPPAへと反映されます。公開されているパッケージは以下のURLで確認できます。

  https://launchpad.net/~groonga/+archive/ubuntu/ppa

Ubuntu用パッケージの公開の取り消し
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LaunchpadのGroongaチームのページで対象のPPAを選択し、バージョン一覧の上にある「View package details」リンクの先で「Delete packages」リンクを辿ると、アップロード済みパッケージを削除できます。
例；[不安定版リポジトリのパッケージの削除用のページ](https://launchpad.net/~groonga/+archive/ubuntu/nightly/+delete-packages)。

WindowsのMSYS2用パッケージのアップロード
----------------------------------------

`MINGW-packages <https://github.com/msys2/MINGW-packages>`_ の、 ``mingw-w64-groonga/PKGBUILD`` を最新にして、プルリクエストを作成します。

MINGW-packagesはforkして自分のリポジトリを作成しておきます。
また、forkしたリポジトリのGitHub Actionsを有効にしておきます。

forkしたリポジトリをローカルにcloneし、upstreamに本家のMINGW-packagesを登録しておきます。この作業は一度だけ行います。

.. code-block:: console

   $ mkdir -p ~/work
   $ git clone --recursive git@github.com:<your-forked-MINGW-packages>.git ~/work/MINGW-packages
   $ git remote add upstream https://github.com/msys2/MINGW-packages.git

以下の手順で必要なファイルの更新と、プルリクエスト用のブランチの作成をします。
``12.0.9`` は最新のGroongaのバージョンを指定します。

.. code-block:: console

   $ cd ~/work/groonga/groonga.clean/packages
   $ ./post-msys2.sh 12.0.9 $HOME/work/MINGW-packages

``post-msys2.sh`` スクリプトは以下の処理を実行します。

* forkしたリポジトリの更新（ ``master`` ブランチを本家のリポジトリの ``master`` にrebase）
* ``master`` ブランチから ``groonga-12.0.9`` ブランチの作成
* ``mingw-w64-groonga/PKGBUILD`` の更新
* forkしたリポジトリに ``groonga-12.0.9`` ブランチをpush

このとき、 ``mingw-w64-groonga/PKGBUILD`` は以下の通り更新されます。

* ``pkgver`` : 指定した最新のGroongaバージョン
* ``pkgrel`` : ``1``
* ``sha256sums`` : 最新の https://packages.groonga.org/source/groonga/groonga-xx.x.x.tar.gz のsha256sum

forkしたリポジトリにて、pushされたブランチのGitHub Actionsが成功していることを確認します。
これで正しくビルドできているかどうかが確認できます。

確認後、本家のMINGW-packagesにプルリクエストを作成します。

過去のプルリクエストの例は以下です。

  https://github.com/msys2/MINGW-packages/pull/14320

プルリクエストがマージされると、MSYS2用のパッケージがリリースされます。

ドキュメントの更新
------------------

``groonga.org`` リポジトリにて次のタスクを実行します。そうすることでタグがプッシュされCIにてドキュメントが更新されます。

.. code-block:: console

   $ cd ${GROONGA_ORG_PATH}
   $ rake release:version:update

Dockerイメージの更新
--------------------

`Docker Hub <https://hub.docker.com/r/groonga/groonga>`_ のGroongaのDockerイメージを更新します。

`GroongaのDockerリポジトリー <https://github.com/groonga/docker>`_ をクローンし、リポジトリーの中のDockerfileを更新します。

以下は、Groongaのバージョンが ``12.0.9`` の場合の例です。作業時には最新のバージョンを指定してください。

.. code-block:: console

   $ mkdir -p ~/work/groonga
   $ rm -rf ~/work/groonga/docker.clean
   $ git clone --recursive git@github.com:groonga/docker.git ~/work/groonga/docker.clean
   $ cd ~/work/groonga/docker.clean
   $ ./update.sh 12.0.9 #Automatically update Dockerfiles and commit changes and create a tag.
   $ git push

`GroongaのDockerリポジトリーのGithub Actions <https://github.com/groonga/docker/actions>`_ が成功しているのを確認してから、タグをpushします。

.. code-block:: console

   $ git push --tags

pushすると、 GroongaのDockerリポジトリーのGithub Actions が Docker HubのGroonga のDockerイメージを自動で更新します。

リリースアナウンスの作成
------------------------

リリースの際にはリリースアナウンスを流して、Groongaを広く通知します。

news.rstに変更点をまとめましたが、それを元にリリースアナウンスを作成します。

リリースアナウンスには以下を含めます。

* インストール方法へのリンク
* リリースのトピック紹介
* リリース変更点へのリンク
* リリース変更点(news.rstの内容)

リリースのトピック紹介では、これからGroongaを使う人へアピールする点や既存のバージョンを利用している人がアップグレードする際に必要な情報を提供します。

非互換な変更が含まれるのであれば、回避方法等の案内を載せることも重要です。

参考までに過去のリリースアナウンスへのリンクを以下に示します。

* [Groonga-talk] [ANN] Groonga 2.0.2

    * http://sourceforge.net/mailarchive/message.php?msg_id=29195195

* [groonga-dev,00794] [ANN] Groonga 2.0.2

    * http://osdn.jp/projects/groonga/lists/archive/dev/2012-April/000794.html

後述しますが、Twitter等でのリリースアナウンスの際はここで用意したアナウンス文の要約を使用します。

Homebrewの更新
--------------

この手順は省略可能です（Homebrewの更新はGroongaプロジェクト本体のリリース要件には含まれません）。

OS Xでのパッケージ管理方法として `Homebrew <http://brew.sh/>`_ があります。

Groongaを簡単にインストールできるようにするために、Homebrewへpull requestを送ります。

  https://github.com/Homebrew/homebrew-core

すでにGroongaのFormulaは取り込まれているので、リリースのたびにFormulaの内容を更新する作業を実施します。

Groonga 3.0.6のときは以下のように更新してpull requestを送りました。

  https://github.com/mxcl/homebrew/pull/21456/files

上記URLを参照するとわかるようにソースアーカイブのurlとsha1チェックサムを更新します。

リリースアナウンス
------------------

作成したリリースアナウンスをメーリングリストへと流します。

* groonga-dev groonga-dev@lists.osdn.me
* Groonga-talk groonga-talk@lists.sourceforge.net

Twitterでリリースアナウンスをする
---------------------------------

BloGroongaのリリースエントリには「リンクをあなたのフォロワーに共有する」ためのツイートボタンがあるので、そのボタンを使ってリリースアナウンスします。(画面下部に配置されている)

このボタンを経由する場合、ツイート内容に自動的にリリースタイトル(「groonga 2.0.8リリース」など)とBloGroongaのリリースエントリのURLが挿入されます。

この作業はBloGroongaの英語版、日本語版それぞれで行います。
あらかじめgroongaアカウントでログインしておくとアナウンスを円滑に行うことができます。

Facebookでリリースアナウンスをする
----------------------------------

FacebookにGroongaグループがあります。
https://www.facebook.com/groonga/

Groongaグループのメンバーになると、個人のアカウントではなく、Groongaグループのメンバーとして投稿できます。
ブログエントリなどをもとに、リリースアナウンスを投稿します。

以上でリリース作業は終了です。

パッケージの署名用のパスフレーズを知りたい
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

パッケージの署名に必要な秘密鍵のパスフレーズについては
リリース担当者向けの秘密鍵を復号したテキストの1行目に記載してあります。
