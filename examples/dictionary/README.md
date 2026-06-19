# 辞書検索ツール

## 名前

Groonga辞書検索ツール

## 説明

様々な辞書ファイルをインポートしてGroongaで検索できるようにします。

## 対応している辞書

現状では下記の辞書に対応しています。

### EDICT

[EDICT](http://www.edrdg.org/jmdict/edict.html)は、Monash大学Jim Breen教授が提供している和英辞書です。下記から入手できます。

http://ftp.edrdg.org/pub/Nihongo/edict2.gz

### GENE95

GENE95は、Kurumiさん(NiftyID: GGD00145)が作成された英和辞書です。下記から入手できます。

http://www.namazu.org/~tsuchiya/sdic/data/gene95.tar.gz

## Groonga HTTPサーバーの起動

次のように新規データベースを作り、かつ、`html`ディレクトリーをドキュメントルートとしてGroonga HTTPサーバーを起動します。ここでは`/tmp/db/db`に新規データベースを作成します。

```bash
mkdir -p /tmp/db
groonga \
  --log-path /tmp/db/groonga.log \
  --query-log-path /tmp/db/query.log \
  --document-root examples/dictionary/html \
  --protocol httl \
  -s \
  -n \
  /tmp/db/db
```

## データベースの初期化

下記のように実行し、辞書データを格納するデータベースファイルを下記のようにして初期化します。

```bash
examples/dictionary/init-db.sh データベースパス名
```

このようにして作成したデータベースについて、様々な辞書のデータをインポートすることができます。

## インポートの方法

### EDICT

以下のように実行します。 edict2.gzは自動でダウンロードします。

```bash
examples/dictionary/edict/edict-import.sh データベースパス名
```

### GENE95

下記のように実行します。 gene95.tar.gzは自動でダウンロードします。

```bash
examples/dictionary/gene95/gene-import.sh データベースパス名
```

## 使い方

http://127.0.0.1:10041/ にアクセスすると検索フォームだけが表示されます。そこにテキストを入力するとオートコンプリートが効いて入力中に検索候補が表示されます。ローマ字で入力してもひらがな・カタカナ・漢字の候補が表示されます。これはGroongaが内部的にローマ字をカタカナに変換し、データベース内に登録されているカタカナのよみがな情報に対して検索しているためです。

また、テキストを入力するとリアルタイムで検索結果を表示するようになっているので「検索」ボタンを押さなくても検索結果を得ることができます。
