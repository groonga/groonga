.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: functions_string_slice

``string_slice``
==================

.. versionadded:: 11.0.2

Summary
-------

``string_slice`` は文字列の部分文字列を抽出します。引数によって部分文字列の抽出方法を使い分けることが出来ます。抽出方法は以下の2つがあります。

* 位置による抽出
* 正規表現による抽出

Groongaでは正規表現にRubyと同じ構文を使います。

この関数で正規表現による抽出をする場合、正規表現のマッチにはインデックスを使用しません。
また、検索で正規表現を使う場合と異なり、マッチ対象のテキストを正規化しません。 :doc:`/reference/regular_expression` も参照してください。

To enable this function, register ``functions/string`` plugin by following the command::

  plugin_register functions/string

Syntax
------

``string_slice`` は2つから4つの引数を指定できます。抽出方法によって指定可能な引数が変わります。

位置による抽出
^^^^^^^^^^^^^^^^^^

::

  string_slice(target, nth[, options])``
  string_slice(target, nth, length[, options])``

``options`` には以下のキーを指定します。すべてのキー・値のペアは省略可能です。::

  {
    "default_value": default_value,
  }

正規表現による抽出
^^^^^^^^^^^^^^^^^^^^^^
 
::

  string_slice(target, regexp, nth_or_name[, options])


``options`` には以下のキーを指定します。すべてのキー・値のペアは省略可能です。::

  {
    "default_value": default_value,
  }


Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_setup_schema.log

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_setup_data.log

位置により抽出する場合の簡単な例です。

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_number.log

正規表現により抽出する場合の簡単な例です。

以下の例では、捕獲式集合 ``(式)`` の番号を指定して抽出しています。

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_number.log

以下の例では、名前付き捕獲式集合 ``(?<name>式)`` の名前を指定して抽出しています。

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_name.log

以下の例では、マッチしなかった場合のデフォルト値を指定しています。

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_regexp_default.log

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/string_slice/usage_string_literal.log

Parameters
----------

位置による抽出
^^^^^^^^^^^^^^^^

必須引数は2つあります。 ``target`` と ``nth`` です。

省略可能引数は2つあります。 ``length`` と ``options`` です。

``target``
~~~~~~~~~~~~~~~~~~~~~~

対象となる文字列または文字列型カラムを指定します。

``nth``
~~~~~~~~~~~~~~~~~~~~~~

``target`` から抽出を開始する位置を指定します。負の値を指定した場合は終端から数えます。

``length``
~~~~~~~~~~~~~~~~~~~~~~

``nth`` から抽出する文字数を指定します。省略時は1です。

``options``
~~~~~~~~~~~~~~~~~~~~~~

以下のキーを指定します。

``default_value``
````````````````````

抽出文字列が存在しなかった場合に返される文字列を指定します。 

省略時は空文字列です。


正規表現による抽出
^^^^^^^^^^^^^^^^^^^^^^^

必須引数は3つあります。 ``target`` と ``regexp`` と ``nth_or_name`` です。

省略可能引数は1つあります。 ``options`` です。

``target``
~~~~~~~~~~~~~~~~~~~~~~

対象となる文字列または文字列型カラムを指定します。

``regexp``
~~~~~~~~~~~~~~~~~~~~~~

正規表現文字列を指定します。

``nth_or_name`` に数値を使用し、かつ1以上の値を指定する場合は、捕獲式集合 ``(式)`` を使用する必要があります。

``nth_or_name`` に名前を使用する場合は、名前付き捕獲式集合 ``(?<name>式), (?'name'式)`` を使用する必要があります。

``nth_or_name``
~~~~~~~~~~~~~~~~~~~~~~

``regexp`` の捕獲式集合を指定します。数値か文字列を指定可能です。

**数値を指定する場合**

``regexp`` の捕獲式集合の番号を指定します。

``regexp`` で指定したパターンに一致した時、 ``nth`` 番目の捕獲式集合の内容が返却されます。

0を指定すると、 ``regexp`` で指定したパターンに一致した全体が返却されます。

**文字列を指定する場合**

``regexp`` の名前付き捕獲式集合の名前を指定します。

``regexp`` で指定したパターンに一致した時、この名前に一致する名前付き捕獲式集合の内容が返却されます。

``options``
~~~~~~~~~~~~~~~~~~~~~~

以下のキーを指定します。

``default_value``
````````````````````

``regexp`` に一致しなかった場合に返される文字列を指定します。 ``nth_or_name`` の値に誤りがある場合もこの値が返却されます。

省略時は空文字列です。

Return value
------------

``string_slice`` は指定した条件で抽出された文字列を返却します。
