.. -*- rst -*-

Command version
===============

概要
----

Groonga1.1からコマンドバージョンという概念が導入されます。コマンドバージョンは、selectやloadなどのGroongaのコマンドの仕様の互換性を表します。Groongaパッケージのバージョンが新しくなったとしても、同一のコマンドバージョンが使用可能であるなら、すべてのコマンドについて互換性が保証されます。コマンドバージョンが異なれば、同じ名前のコマンドであっても、動作に互換性がない可能性があります。

あるバージョンのGroongaは、二つのコマンドバージョンを同時にサポートするようになります。
使用するコマンドバージョンは、groongaを起動する際のコマンドラインオプションないしコンフィグファイルにdefault-commnad-versionパラメータを与えることによって指定できます。また、個々のコマンドを実行する際に、command_versionパラメータを与えることによっても指定することができます。

コマンドバージョンは1からはじまり、更新されるたびに1ずつ大きくなります。現状のGroongaのコマンドの仕様はcommand-version 1という扱いになります。次回提供するGroongaは、command-version 1とcommand-version 2の二つをサポートすることになります。

バージョンの位置づけ
--------------------

あるバージョンのGroongaにおいてサポートされるコマンドバージョンは、develop, stable,deprecatedのいずれかの位置づけとなります。

develop
  まだ開発中であり、仕様が変更される可能性があります。

stable
  使用可能であり仕様も安定しています。その時点で使用することが推奨されます。

deprecated
  使用可能であり仕様も安定していますが、廃止予定であり使用が推奨されません。

あるバージョンのGroongaがサポートする二つのコマンドバージョンのうち、いずれか一つが必ずstableの位置づけとなります。残りの一つは、developないしdeprecatedとなります。

たとえば下記のようにGroongaのサポートするコマンドバージョンは推移します。::

  groonga1.1: command-version1=stable     command-version2=develop
  groonga1.2: command-version1=deprecated command-version2=stable
  groonga1.3: command-version2=stable     command-version3=develop
  groonga1.4: command-version2=deprecated command-version3=stable
  groonga1.5: command-version3=stable     command-version4=develop

あるコマンドバージョンははじめにdevelop扱いとしてリリースされ、やがてstableに移行します。
その後二世代経過するとそのコマンドバージョンはdeprecated扱いとなります。さらに次のコマンドバージョンがリリースされると、deprecatedだったコマンドバージョンはサポート対象外となります。

default-commnad-versionパラメータやcommand_versionパラメータを指定せずにgroongaコマンドを実行した際には、その時点でstableであるコマンドバージョンが指定されたものとみなします。

groongaプロセス起動時に、default-command-versionパラメータにstable扱いでないコマンドバージョンを指定した場合には、警告メッセージがログファイルに出力されます。また、サポート範囲外のコマンドバージョンを指定した場合にはエラーとなり、プロセスは速やかに停止します。

コマンドバージョンの指定方法
----------------------------

コマンドバージョンの指定方法はgroonga実行モジュールの引数として指定する方法と各コマンドの引数として指定する方法があります。

default-command-versionパラメータ
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

groonga実行モジュールの引数としてdefault-command-versionパラメータを指定できます。
(configファイルの中に指定することも可能です)

実行例::

  groonga --default-command-version 1

そのプロセスで実行するすべてのコマンドについて、デフォルトのコマンドバージョンとして指定されたバージョンを使用します。指定されたコマンドバージョンがstableであった場合にはなんのメッセージも表示されずそのまま起動します。指定されたコマンドバージョンがdevelopあるいはdeprecatedであった場合には、groonga.logファイルに警告メッセージを出力します。指定されたコマンドバージョンがサポート対象外であった場合には標準エラー出力にエラーメッセージを出力し、プロセスは速やかに終了します。

command_versionパラメータ
^^^^^^^^^^^^^^^^^^^^^^^^^

select,loadなどのすべてのgroongaコマンドにcommand_versionが指定できます。

実行例::

  select --command_version 1 --table tablename

指定されたコマンドバージョンでコマンドを実行します。指定されたコマンドバージョンがサポート対象外であった場合にはエラーが返されます。command-versionが指定されなかった場合は、当該プロセス起動時にdefault-command-versionに指定した値が指定されたものとみなします。
