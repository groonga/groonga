.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tutorial-match-columns

match_columnsパラメータ
=======================

複数のカラムを対象とした全文検索
--------------------------------

Groongaでは、複数のカラムを対象とした全文検索を行うことができます。例えば、ブログのテーブルで、タイトルと内容とがそれぞれ別のカラムに入ったものがあるとしましょう。「タイトルもしくは内容に特定の単語を含む」検索を行いたいとします。

この場合、2つのインデックス作成方式があります。1つは、それぞれのカラムに1つずつインデックスを付与する方式です。もう1つは、複数のカラムに対して1つのインデックスを付与する方式です。Groongaでは、どちらの形式のインデックスが存在している場合でも、同一の記法で全文検索を行うことができます。

カラムごとにインデックスを付与する場合
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Blog1テーブルを作り、タイトル文字列のtitleカラム、本文のmessageカラムを追加しています。
インデックス用のIndexBlog1テーブルも作り、titleカラムのインデックス用にindex_titleカラム、messageカラムのインデック用にindex_messageカラムと、それぞれ1カラムごとに1つずつ追加しています。

.. groonga-command
.. include:: ../example/tutorial/match_columns-1.log
.. table_create --name Blog1 --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Blog1 --name title --flags COLUMN_SCALAR --type ShortText
.. column_create --table Blog1 --name message --flags COLUMN_SCALAR --type ShortText
.. table_create --name IndexBlog1 --flags TABLE_PAT_KEY|KEY_NORMALIZE --key_type ShortText --default_tokenizer TokenBigram
.. column_create --table IndexBlog1 --name index_title --flags COLUMN_INDEX|WITH_POSITION --type Blog1 --source title
.. column_create --table IndexBlog1 --name index_message --flags COLUMN_INDEX|WITH_POSITION --type Blog1 --source message
.. load --table Blog1
.. [
.. {"_key":"grn1","title":"groonga test","message":"groonga message"},
.. {"_key":"grn2","title":"baseball result","message":"rakutan eggs 4 - 4 groonga moritars"},
.. {"_key":"grn3","title":"groonga message","message":"none"}
.. ]

match_columnsオプションで、検索対象のカラムを複数指定することが出来ます。検索する文字列はqueryオプションで指定します。これを使うことで、タイトルと本文を全文検索することができます。

実際に検索してみましょう。

.. groonga-command
.. include:: ../example/tutorial/match_columns-2.log
.. select --table Blog1 --match_columns title||message --query groonga
.. select --table Blog1 --match_columns title||message --query message
.. select --table Blog1 --match_columns title --query message

複数のカラムにまたがったインデックスを付与する場合
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

内容は上の例とほぼ同じですが、titleとmessageの2つのカラムに対するインデックスが共通になっており、インデックスカラムが1つしかありません。

共通のインデックスを用いても、titleカラムのみでの検索、messageカラムのみでの検索、titleもしくはmessageカラムでの検索、全ての検索を行うことができます。


.. groonga-command
.. include:: ../example/tutorial/match_columns-3.log
.. table_create --name Blog2 --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Blog2 --name title --flags COLUMN_SCALAR --type ShortText
.. column_create --table Blog2 --name message --flags COLUMN_SCALAR --type ShortText
.. table_create --name IndexBlog2 --flags TABLE_PAT_KEY|KEY_NORMALIZE --key_type ShortText --default_tokenizer TokenBigram
.. column_create --table IndexBlog2 --name index_blog --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Blog2 --source title,message
.. load --table Blog2
.. [
.. {"_key":"grn1","title":"groonga test","message":"groonga message"},
.. {"_key":"grn2","title":"baseball result","message":"rakutan eggs 4 - 4 groonga moritars"},
.. {"_key":"grn3","title":"groonga message","message":"none"}
.. ]

実際に検索してみましょう。結果は上の例と同じになります。

.. groonga-command
.. include:: ../example/tutorial/match_columns-4.log
.. select --table Blog2 --match_columns title||message --query groonga
.. select --table Blog2 --match_columns title||message --query message
.. select --table Blog2 --match_columns title --query message

.. note::

   There may be a question that "which is the better solution for indexing."
   It depends on the case.

   * Indexes for each column - The update performance tends to be better than multiple colum index because there is enough buffer for updating. On the other hand, the efficiency of disk usage is not so good.
   * Indexes for multiple column - It saves disk usage because it shares common buffer. On the other hand, the update performance is not so good.


インデックス名を指定した全文検索
--------------------------------

執筆中です。

.. TODO: match_columnsにインデックス名を指定しての検索についても触れる。


.. _nested-index-search:

Nested index search among related table by column index
--------------------------------------------------------

If there are relationships among multiple table with column index,
you can search multiple table by specifying reference column name.

Here is the concrete example.

There are tables which store blog articles, comments for articles.
The table which stores articles has columns for article and comment.
And the comment column refers Comments table.
The table which stores comments has columns for comment and column index to article table.

if you want to search the articles which contain specified keyword in comment,
you need to execute fulltext search for table of comment, then search the records which contains fulltext search results.

But, you can search the records by specifying the reference column index at once.

Here is the sample schema.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-schema.log
.. table_create Comments TABLE_HASH_KEY UInt32
.. column_create Comments content COLUMN_SCALAR ShortText
.. table_create Articles TABLE_NO_KEY
.. column_create Articles content COLUMN_SCALAR Text
.. column_create Articles comment COLUMN_SCALAR Comments
.. table_create Lexicon TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
.. column_create Lexicon articles_content COLUMN_INDEX|WITH_POSITION Articles content
.. column_create Lexicon comments_content COLUMN_INDEX|WITH_POSITION Comments content
.. column_create Comments article COLUMN_INDEX Articles comment

Here is the sample data.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-data.log
.. load --table Comments
.. [
.. {"_key": 1, "content": "I'm using groonga too!"},
.. {"_key": 2, "content": "I'm using groonga and mroonga!"},
.. {"_key": 3, "content": "I'm using mroonga too!"}
.. ]
.. load --table Articles
.. [
.. {"content": "Groonga is fast!", "comment": 1},
.. {"content": "Groonga is useful!"},
.. {"content": "Mroonga is fast!", "comment": 3}
.. ]

You can write the query that search the records which contains specified keyword as a comment, then fetch the articles which refers to it.

Query for searching the records described above::

  select Articles --match_columns comment.content --query groonga --output_columns "_id, _score, *"

You need to concatenate comment column of Articles table and content column of Comments table with period(.) as --match_columns arguments.

At first, this query execute fulltext search from content of Comments table, then fetch the records of Articles table which refers to already searched records of Comments table.
(Because of this, if you comment out the query which create column index 'article' of Comments table, you can't get intended search results.)

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-select.log
.. select Articles --match_columns comment.content --query groonga --output_columns "_id, _score, *"

Now, you can search articles which contains specific keywords as a comment.

The feature of nested index search is not limited to the relationship between two table only.

Here is the sample schema similar to previous one. The difference is added table which express 'Reply' and relationship is extended to three tables.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-schema-with-three-relationship.log
.. table_create Replies2 TABLE_HASH_KEY UInt32
.. column_create Replies2 content COLUMN_SCALAR ShortText
.. table_create Comments2 TABLE_HASH_KEY UInt32
.. column_create Comments2 content COLUMN_SCALAR ShortText
.. column_create Comments2 comment COLUMN_SCALAR Replies2
.. table_create Articles2 TABLE_NO_KEY
.. column_create Articles2 content COLUMN_SCALAR Text
.. column_create Articles2 comment COLUMN_SCALAR Comments2
.. table_create Lexicon2 TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
.. column_create Lexicon2 articles_content COLUMN_INDEX|WITH_POSITION Articles2 content
.. column_create Lexicon2 comments_content COLUMN_INDEX|WITH_POSITION Comments2 content
.. column_create Lexicon2 replies_content COLUMN_INDEX|WITH_POSITION Replies2 content
.. column_create Comments2 article COLUMN_INDEX Articles2 comment
.. column_create Replies2 reply_to COLUMN_INDEX Comments2 comment

Here is the sample data.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-data-with-three-relationship.log
.. load --table Replies2
.. [
.. {"_key": 1, "content": "I'm using rroonga too!"},
.. {"_key": 2, "content": "I'm using groonga and mroonga and rroonga!"},
.. {"_key": 3, "content": "I'm using nroonga too!"}
.. ]
.. load --table Comments2
.. [
.. {"_key": 1, "content": "I'm using groonga too!", "comment": 1},
.. {"_key": 2, "content": "I'm using groonga and mroonga!", "comment": 2},
.. {"_key": 3, "content": "I'm using mroonga too!"}
.. ]
.. load --table Articles2
.. [
.. {"content": "Groonga is fast!", "comment": 1},
.. {"content": "Groonga is useful!", "comment": 2},
.. {"content": "Mroonga is fast!", "comment": 3}
.. ]

Query for searching the records described above::

  select Articles2 --match_columns comment.content --query mroonga --output_columns "_id, _score, *"
  select Articles2 --match_columns comment.comment.content --query mroonga --output_columns "_id, _score, *"

The first query searches 'mroonga' from Comments2 table, the second one searches 'mroonga' from Replies2 and Comment2 table by using reference column index.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-select-with-three-relationship.log
.. select Articles2 --match_columns comment.content --query mroonga --output_columns "_id, _score, *"
.. select Articles2 --match_columns comment.comment.content --query mroonga --output_columns "_id, _score, *"

As a result, the first query matches two article because of Comments2 table has two records which contains 'mroonga' as keyword.

On the other hand, the second one matches one article only because of Replies2 table has only one record which contains 'mroonga' as keyword, and there is one record which contains same keyword and refers to the record in Comments2 table.

インデックスの重み
------------------

執筆中です。

.. TODO: match_columnsの重み指定機能についても触れる。
