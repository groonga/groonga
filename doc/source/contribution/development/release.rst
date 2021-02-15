.. -*- rst -*-

.. highlightlang:: none

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
* CUTTER_DIR=$HOME/work/cutter
* CUTTER_SOURCE_PATH=$HOME/work/cutter/cutter
* APACHE_ARROW_REPOSITORY=$HOME/work/apache/arrow
* PACKAGES_GROONGA_ORG_REPOSITORY=$HOME/work/groonga/packages.groonga.org

最初の1回だけ行う手順
**********************


ビルド環境の準備
----------------

以下にGroongaのリリース作業を行うために事前にインストール
しておくべきパッケージを示します。

なお、ビルド環境としては Debian GNU/Linux (sid)を前提として説明しているため、その他の環境では適宜読み替えて下さい。::

    % sudo apt-get install -V debootstrap createrepo rpm mercurial python-docutils python-jinja2 ruby-full mingw-w64 g++-mingw-w64 mecab libmecab-dev nsis gnupg2 dh-autoreconf bison

また、Sphinxは常に最新のバージョンを使う事を推奨します。 ``pip3`` を使用して最新のSphinxをインストールするようにして下さい。::

    % pip3 install --upgrade sphinx

また、rubyのrakeパッケージを以下のコマンドによりインストールします。::

    % sudo gem install rake

パッケージ署名用秘密鍵のインポート
----------------------------------

リリース作業ではRPMパッケージに対する署名を行います。
その際、パッケージ署名用の鍵が必要です。

Groongaプロジェクトでは署名用の鍵をリリース担当者の公開鍵で暗号化してリポジトリのpackages/ディレクトリ以下へと登録しています。新しいリリース担当者に任命されたばかりで、まだ自分用に暗号化された鍵が無い場合には、他のリリース担当者に依頼して署名用の鍵を暗号化してもらって下さい。

リリース担当者はリポジトリに登録された秘密鍵を復号した後に鍵のインポートを以下のコマンドにて行います。::

    % cd packages
    % gpg --decrypt release-key-secret.asc.gpg.(担当者) > (復号した鍵
    ファイル)
    % gpg --import  (復号した鍵ファイル)

鍵のインポートが正常終了すると gpg --list-keys でGroongaの署名用の鍵を確認することができます。::

    pub   1024R/F10399C0 2012-04-24
    uid                  groonga Key (groonga Official Signing Key)
    <packages@groonga.org>
    sub   1024R/BC009774 2012-04-24

鍵をインポートしただけでは使用することができないため、インポートした鍵に対してtrust,signを行う必要があります。

以下のコマンドを実行して署名を行います。(途中の選択肢は省略)::

    % gpg --edit-key packages@groonga.org
    gpg> trust
    gpg> sign
    gpg> save
    gpg> quit

この作業は、新規にリリースを行うことになった担当者やパッケージに署名する鍵に変更があった場合などに行います。

PPA用の鍵の登録
---------------

この作業は、新規にリリースを行うことになった担当者のみ行います。

[Launchpad](https://launchpad.net/)にアカウントを作成し、自分の普段使いの秘密鍵を登録した上で、他のリリース担当者に依頼して[Groongaチーム](https://launchpad.net/~groonga)に追加してもらって下さい。


リリース作業用ディレクトリの作成
--------------------------------

Groongaのリリース作業ではリリース専用の環境下(コンパイルフラグ)でビルドする必要があります。

リリース時と開発時でディレクトリを分けずに作業することもできますが、誤ったコンパイルフラグでリリースしてしまう危険があります。

そのため、以降の説明では$GROONGA_DIR以下のディレクトリにリリース用の作業ディレクトリ(groonga.clean)としてソースコードをcloneしたものとして説明します。


毎回のリリースで行う手順
************************

Groongaのソースコード取得
-------------------------

リリース用のクリーンな状態でソースコードを取得するために$GROONGA_DIRにて以下のコマンドを実行します。::

    % git clone --recursive git@github.com:groonga/groonga.git groonga.clean

この作業はリリース作業ごとに行います。

Groongaのウェブサイトの取得
---------------------------

GroongaのウェブサイトのソースはGroonga同様にGitHubにリポジトリを置いています。

リリース作業では後述するコマンド(make update-latest-release)にてトップページのバージョンを置き換えることができるようになっています。

Groongaのウェブサイトのソースコードを$GROONGA_ORG_PATHとして取得するためには、$GROONGA_DIRにて以下のコマンドを実行します。::

    % git clone git@github.com:groonga/groonga.org.git

これで、$GROONGA_ORG_PATHにgroonga.orgのソースを取得できます。

cutterのソースコード取得
------------------------

Groongaのリリース作業では、cutterに含まれるスクリプトを使用しています。

そこであらかじめ用意しておいた$HOME/work/cutterディレクトリにてcutterのソースコードを以下のコマンドにて取得します。::

    % git clone git@github.com:clear-code/cutter.git

これで、$CUTTER_SOURCE_PATHディレクトリにcutterのソースを取得できます。

configureスクリプトの生成
-------------------------

Groongaのソースコードをcloneした時点ではconfigureスクリプトが含まれておらず、そのままmakeコマンドにてビルドすることができません。

$GROONGA_CLONE_DIRにてautogen.shを以下のように実行します。::

    % sh autogen.sh

このコマンドの実行により、configureスクリプトが生成されます。

configureスクリプトの実行
-------------------------

Makefileを生成するためにconfigureスクリプトを実行します。

リリース用にビルドするためには以下のオプションを指定してconfigureを実行します。::

    % ./configure \
          --prefix=/tmp/local \
          --with-launchpad-uploader-pgp-key=(Launchpadに登録したkeyID) \
          --with-groonga-org-path=$HOME/work/groonga/groonga.org \
          --enable-document \
          --with-ruby \
          --enable-mruby \
          --with-cutter-source-path=$HOME/work/cutter/cutter

configureオプションである--with-groonga-org-pathにはGroongaのウェブサイトのリポジトリをcloneした場所を指定します。

configureオプションである--with-cutter-source-pathにはcutterのソースをcloneした場所を指定します。

以下のようにGroongaのソースコードをcloneした先からの相対パスを指定することもできます。::

    % ./configure \
          --prefix=/tmp/local \
          --with-launchpad-uploader-pgp-key=(Launchpadに登録したkeyID) \
          --with-groonga-org-path=../groonga.org \
          --enable-document \
          --with-ruby \
          --enable-mruby \
          --with-cutter-source-path=../../cutter/cutter

あらかじめpackagesユーザでpackages.groonga.orgにsshログインできることを確認しておいてください。

ログイン可能であるかの確認は以下のようにコマンドを実行して行います。::

    % ssh packages@packages.groonga.org

デバッグ用や開発用のパッケージをテスト用に公開する時は、 ``--with-launchpad-ppa=groonga-nightly`` を指定して不安定版のリポジトリにアップロードするように指定します。::

    % ./configure \
          --with-launchpad-ppa=groonga-nightly \
          --prefix=/tmp/local \
          --with-launchpad-uploader-pgp-key=(Launchpadに登録したkeyID) \
          --with-groonga-org-path=$HOME/work/groonga/groonga.org \
          --enable-document \
          --with-ruby \
          --enable-mruby \
          --with-cutter-source-path=$HOME/work/cutter/cutter

新任のリリース担当者は必ず、この方法でPPAのリポジトリにパッケージをアップロードできる事を確認しておいてください。

PPAのリポジトリは、同名のパッケージを上書いてアップロードできないので、不安定版のリポジトリでビルドできることを確認してから、安定版のリポジトリへアップロードするようにしてください。

変更点のまとめ
--------------

前回リリース時からの変更点を$GROONGA_CLONE_DIR/doc/source/news.rst（英語）にまとめます。
ここでまとめた内容についてはリリースアナウンスにも使用します。

前回リリースからの変更履歴を参照するには以下のコマンドを実行します。::

   % git log -p --reverse $(git tag | tail -1)..

ログを^commitで検索しながら、以下の基準を目安として変更点を追記していきます。

含めるもの

* ユーザへ影響するような変更
* 互換性がなくなるような変更

含めないもの

* 内部的な変更(変数名の変更やらリファクタリング)

make update-latest-releaseの実行
--------------------------------

make update-latest-releaseコマンドでは、OLD_RELEASE_DATEに前回のリリースの日付を、NEW_RELEASE_DATEに次回リリースの日付（未来の日付）を指定します。

2.0.2のリリースを行った際は以下のコマンドを実行しました。::
::

   % make update-latest-release OLD_RELEASE=2.0.1 OLD_RELEASE_DATE=2012-03-29 NEW_RELEASE_DATE=2012-04-29

これにより、clone済みのGroongaのWebサイトのトップページのソース(index.html,ja/index.html)やRPMパッケージのspecファイルのバージョン表記などが更新されます。

make update-examplesの実行
--------------------------

ドキュメントに埋め込まれている実行結果を更新するために、以下のコマンドを実行します。::

    % cd doc && make update-examples

doc/source/examples以下が更新されるので、それらをコミットします。

make update-filesの実行
-----------------------

ロケールメッセージの更新や変更されたファイルのリスト等を更新するために以下のコマンドを実行します。::

    % make update-files

make update-filesを実行すると新規に追加されたファイルなどが各種.amファイルへとリストアップされます。

リリースに必要なファイルですので漏れなくコミットします。

make update-poの実行
--------------------

ドキュメントの最新版と各国語版の内容を同期するために、poファイルの更新を以下のコマンドにて実行します。::

    % make update-po

make update-poを実行すると、doc/locale/ja/LC_MESSAGES以下の各種.poファイルが更新されます。

poファイルの翻訳
----------------

make update-poコマンドの実行により更新した各種.poファイルを翻訳します。

翻訳結果をHTMLで確認するために、以下のコマンドを実行します。::

    % make -C doc/locale/ja html
    % make -C doc/locale/en html

修正が必要な箇所を調べて、 ``***.edit`` というファイルを適宜修正します。::

    % cd groonga/doc/locale
    % git diff

``***.edit`` というファイルの編集中は、翻訳元のファイルは絶対に編集しないで下さい（編集すると、``***.edit`` に加えた変更が make update-po の実行時に失われます）。
ファイルを編集したら、再度poファイルとHTMLを更新するために以下のコマンドを実行します。::

    % make update-po
    % make -C doc/locale/ja html
    % make -C doc/locale/en html

確認が完了したら、翻訳済みpoファイルをコミットします。

各種テストの確認
----------------

リリース用のタグを設定する前に、以下のテストが全てパスしているかを確認します。
タグを設定してから問題が発覚すると、再度リリースすることになってしまうので、タグを設定する前に問題がないか確認します。

* `GitHub Actions <https://github.com/groonga/groonga/actions?query=workflow%3APackage>`_
* `TravisCI <https://travis-ci.org/github/groonga/groonga>`_
* `AppVeyor <https://ci.appveyor.com/project/groonga/groonga>`_

テストやパッケージの作成に失敗していたら、原因を特定して修正します。

リリースタグの設定
------------------

リリース用のタグを打つには以下のコマンドを実行します。::

    % make tag
    % git push --tags origin

.. note::
   タグを打った後にconfigureを実行することで、ドキュメント生成時のバージョン番号に反映されます。

リリース用アーカイブファイルの作成
----------------------------------

リリース用のソースアーカイブファイルを作成するために以下のコマンドを$GROONGA_CLONE_DIRにて実行します。::

    % make dist

これにより$GROONGA_CLONE_DIR/groonga-(バージョン).tar.gzが作成されます。

.. note::
   タグを打つ前にmake distを行うとversionが古いままになることがあります。
   するとgroonga --versionで表示されるバージョン表記が更新されないので注意が必要です。
   make distで生成したtar.gzのversionおよびversion.shがタグと一致することを確認するのが望ましいです。

リリース用アーカイブファイルのアップロード
------------------------------------------

Groongaのリリース用アーカイブファイルは、MroongaやPGroonga、Rroonga等関連プロダクトのリリースにも使用します。
生成でき次第アップロードしておくと、関連プロダクトのリリース作業がしやすくなります。

リリース用のアーカイブファイルは以下の手順でアップロードします。

アップロードはrsyncコマンドで行います。
以下のコマンドを実行して、現在のリポジトリーと同期します。::

    % cd packages/source
    % make download

これにより過去にリリースしたソースアーカイブ(.tar.gz)が
packages/source/filesディレクトリ以下へとダウンロードされます。

その後、以下のコマンドを実行してリリース用ソースアーカイブをアップロードします。::

    % cd packages/source
    % make upload

アップロードが正常終了すると、リリース用ソースアーカイブがpackages.groonga.orgへと反映されます。

パッケージのビルド
------------------

リリース用のアーカイブファイルができたので、パッケージ化する作業を行います。

パッケージ化作業は以下の3種類を対象に行います。

* Debian系(.deb)
* Red Hat系(.rpm)
* Windows系(.exe,.zip)

パッケージのビルドではいくつかのサブタスクから構成されています。

Debian系パッケージのビルドとアップロード
----------------------------------------

環境変数 ``APACHE_ARROW_REPOSITORY`` にapache/arrowのリポジトリをcloneしたパスを指定します。

現在サポートしているOSのバージョンは以下の通りです。

* Debian GNU/Linux

  * stretch i386/amd64
  * buster i386/amd64

正常にビルドが終了すると$GROONGA_CLONE_DIR/packages/apt/repositories配下に.debパッケージが生成されます。

debパッケージをビルドするには以下のコマンドを実行します。::

    % cd packages
    % rake apt

パッケージのビルドが完了したら、以下のコマンドを実行してビルドしたパッケージをpackages.groonga.orgへアップロードします。::

    % rake apt:release

この段階では、ビルドしたパッケージはまだ未署名なので、$PACKAGES_GROONGA_ORG_REPOSITORYに移動し、以下のコマンドを実行します。::

    % rake apt

上記のコマンドを実行することで、リポジトリーの同期、パッケージの署名、リポジトリーの更新、アップロードまで実行できます。

Ubuntu用パッケージのアップロード
--------------------------------

Ubuntu向けパッケージの作成には、作業マシン上にGroongaのビルドに必要な依存ソフトウェア一式がインストールされている必要があります。以下のようにしてインストールしておいて下さい。::

    % sudo apt build-dep groonga

Ubuntu向けのパッケージのアップロードには以下のコマンドを実行します。::

    % cd packages
    % rake ubuntu

現在サポートされているのは以下の通りです。

* Xenial i386/amd64
* Bionic i386/amd64
* Disco  i386/amd64
* Focal  amd64

アップロードが正常終了すると、launchpad.net上でビルドが実行され、ビルド結果がメールで通知されます。ビルドに成功すると、リリース対象のパッケージがlaunchpad.netのGroongaチームのPPAへと反映されます。公開されているパッケージは以下のURLで確認できます。

  https://launchpad.net/~groonga/+archive/ubuntu/ppa

Ubuntu用パッケージの公開の取り消し
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LaunchpadのGroongaチームのページで対象のPPAを選択し、バージョン一覧の上にある「View package details」リンクの先で「Delete packages」リンクを辿ると、アップロード済みパッケージを削除できます。
例；[不安定版リポジトリのパッケージの削除用のページ](https://launchpad.net/~groonga/+archive/ubuntu/nightly/+delete-packages)。


Red Hat系パッケージのビルドとアップロード
-----------------------------------------

現在サポートしているOSのバージョンは以下の通りです。

* centos-6 x86_64
* centos-7 x86_64
* centos-8 x86_64

ビルドが正常終了すると$GROONGA_CLONE_DIR/packages/yum/repositories配下にRPMパッケージが生成されます。

* repositories/yum/centos/6/x86_64/Packages
* repositories/yum/centos/7/x86_64/Packages
* repositories/yum/centos/8/x86_64/Packages

rpmパッケージをビルドするには以下のコマンドを実行します。::

    % cd packages
    % rake yum

パッケージのビルドが完了したら、以下のコマンドを実行してビルドしたパッケージをpackages.groonga.orgへアップロードします。::

    % rake yum:release

この段階では、ビルドしたパッケージはまだ未署名なので、$PACKAGES_GROONGA_ORG_REPOSITORYに移動し、以下のコマンドを実行します。::

    % rake yum

上記のコマンドを実行することで、リポジトリーの同期、パッケージの署名、リポジトリーの更新、アップロードまで実行できます。

Windows用パッケージのビルドとアップロード
-----------------------------------------

Windowsパッケージのビルドに必要なファイルを以下のコマンドを実行してダウンロードします。::

    % cd packages/windows
    % make download

これにより、Groongaのインストーラやzipアーカイブが packages/windows以下へとダウンロードされます。

次に、以下のコマンドを実行してWindowsパッケージをビルドします。::

    % cd packages/windows
    % make build
    % make package
    % make installer

make buildでクロスコンパイルを行います。
正常に終了するとdist-x64/dist-x86ディレクトリ以下にx64/x86バイナリを作成します。

make packageが正常に終了するとzipアーカイブをfilesディレクトリ以下に作成します。

make installerが正常に終了するとWindowsインストーラをfilesディレクトリ以下に作成します。

パッケージのビルドが完了したら、以下のコマンドを実行してビルドしたパッケージをpackages.groonga.orgへアップロードします。::

    % cd packages/windows
    % make upload

パッケージの動作確認
--------------------

.. note::
   パッケージの動作確認はGitHub Actions上で自動で実施できるように改良中で、以下の手順は今後削除する予定です。

ビルドしたパッケージに対しリリース前の動作確認を行います。

Debian系もしくはRed Hat系の場合には本番環境へとアップロードする前にローカルのaptないしyumのリポジトリを参照して正常に更新できることを確認します。

ここでは以下のようにrubyを利用してリポジトリをwebサーバ経由で参照できるようにします。

yumの場合::

    % ruby -run -e httpd -- packages/yum/repositories
    % yum update
    ...

aptの場合::

    % ruby -run -e httpd -- packages/apt/repositories
    % sudo apt update
    ...

grntestの準備
~~~~~~~~~~~~~

TravisCIの結果が正常であれば、この手順はスキップして構いません。
grntestを実行するためにはGroongaのテストデータとgrntestのソースが必要です。

まずGroongaのソースを任意のディレクトリへと展開します。::

    % tar zxvf groonga-(バージョン).tar.gz

次にGroongaのtest/functionディレクトリ以下にgrntestのソースを展開します。
つまりtest/function/grntestという名前でgrntestのソースを配置します。::

    % ls test/function/grntest/
    README.md  binlib  license  test

grntestの実行方法
~~~~~~~~~~~~~~~~~

grntestではGroongaコマンドを明示的に指定することができます。
後述のパッケージごとのgrntestによる動作確認では以下のようにして実行します。::

    % GROONGA=(groongaのパス指定) test/function/run-test.sh

最後にgrntestによる実行結果が以下のようにまとめて表示されます。::

    55 tests, 52 passes, 0 failures, 3 not checked tests.
    94.55% passed.

grntestでエラーが発生しないことを確認します。


Debian系の場合
~~~~~~~~~~~~~~

Debian系の場合の動作確認手順は以下の通りとなります。

* 旧バージョンをテスト環境へとインストールする
* テスト環境の/etc/hostsを書き換えてpackages.groonga.orgがホストを
  参照するように変更する
* ホストでwebサーバを起動してドキュメントルートをビルド環境のもの
  (repositories/apt/packages)に設定する
* アップグレード手順を実行する
* grntestのアーカイブを展開してインストールしたバージョンでテストを実
  行する
* grntestの正常終了を確認する


Red Hat系の場合
~~~~~~~~~~~~~~~

Red Hat系の場合の動作確認手順は以下の通りとなります。

* 旧バージョンをテスト環境へとインストール
* テスト環境の/etc/hostsを書き換えてpackages.groonga.orgがホストを参照するように変更する
* ホストでwebサーバを起動してドキュメントルートをビルド環境のもの(packages/yum/repositories)に設定する
* アップグレード手順を実行する
* grntestのアーカイブを展開してインストールしたバージョンでテストを実行する
* grntestの正常終了を確認する


Windows向けの場合
~~~~~~~~~~~~~~~~~

* テスト環境で新規インストール/上書きインストールを行う
* grntestのアーカイブを展開してインストールしたバージョンでテストを実行する
* grntestの正常終了を確認する

zipアーカイブも同様にしてgrntestを実行し動作確認を行います。

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


blogroonga(ブログ)の更新
------------------------

http://groonga.org/blog/ および http://groonga.org/blog/ にて公開されているリリース案内を作成します。

基本的にはリリースアナウンスの内容をそのまま記載します。

cloneしたWebサイトのソースに対して以下のファイルを新規追加します。

* groonga.org/en/_post/(リリース日)-release.md
* groonga.org/ja/_post/(リリース日)-release.md


編集した内容をpushする前に確認したい場合にはJekyllおよびRedCloth（Textileパーサー）、RDiscount（Markdownパーサー）、JavaScript interpreter（therubyracer、Node.jsなど）が必要です。
インストールするには以下のコマンドを実行します。::

    % sudo gem install jekyll jekyll-paginate RedCloth rdiscount therubyracer

jekyllのインストールを行ったら、以下のコマンドでローカルにwebサーバを起動します。::

    % jekyll serve --watch

あとはブラウザにてhttp://localhost:4000にアクセスして内容に問題がないかを確認します。

.. note::
   記事を非公開の状態でアップロードするには.mdファイルのpublished:をfalseに設定します。::

    ---
    layout: post.en
    title: Groonga 2.0.5 has been released
    published: false
    ---


ドキュメントのアップロード
--------------------------

doc/source以下のドキュメントを更新、翻訳まで完了している状態で、ドキュメントのアップロード作業を行います。

そのためにはまず ``groonga`` のリポジトリをカレントディレクトリにして以下のコマンドを実行します。::

    % GROONGA_VERSION=$(git tag --list | tail -n 1 | tr -d v)
    % make update-document DOCUMENT_VERSION=$GROONGA_VERSION DOCUMENT_VERSION_FULL=$GROONGA_VERSION

ここでは最新のtagに基づいてリリースバージョンを調べ、明示的にそのバージョンを指定してドキュメントを更新するようにしています。
これによりcloneしておいたgroonga.orgのdoc/locale以下に更新したドキュメントがコピーされます。

生成されているドキュメントに問題のないことを確認できたら、コミット、pushしてgroonga.orgへと反映します。

また、``groonga.org`` リポジトリの ``_config.yml`` に最新リリースのバージョン番号と日付を表す情報の指定があるので、これらも更新します。::

    groonga_version: x.x.x
    groonga_release_date: xxxx-xx-xx


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

blogroongaのリリースエントリには「リンクをあなたのフォロワーに共有する」ためのツイートボタンがあるので、そのボタンを使ってリリースアナウンスします。(画面下部に配置されている)

このボタンを経由する場合、ツイート内容に自動的にリリースタイトル(「groonga 2.0.8リリース」など)とblogroongaのリリースエントリのURLが挿入されます。

この作業はblogroongaの英語版、日本語版それぞれで行います。
あらかじめgroongaアカウントでログインしておくとアナウンスを円滑に行うことができます。

Facebookでリリースアナウンスをする
----------------------------------

FacebookにGroongaグループがあります。
https://www.facebook.com/groonga/

Groongaグループのメンバーになると、個人のアカウントではなく、Groongaグループのメンバーとして投稿できます。
ブログエントリなどをもとに、リリースアナウンスを投稿します。

以上でリリース作業は終了です。

リリース後にやること
--------------------

リリースアナウンスを流し終えたら、次期バージョンの開発が始まります。

* Groonga のbase_versionの更新

Groonga バージョン更新
~~~~~~~~~~~~~~~~~~~~~~

$GROONGA_CLONE_DIRにて以下のコマンドを実行します。::

    % make update-version NEW_VERSION=2.0.6

これにより$GROONGA_CLONE_DIR/base_versionが更新されるのでコミットしておきます。

.. note::
   base_versionはtar.gzなどのリリース用のファイル名で使用します。

パッケージの署名用のパスフレーズを知りたい
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

パッケージの署名に必要な秘密鍵のパスフレーズについては
リリース担当者向けの秘密鍵を復号したテキストの1行目に記載してあります。

