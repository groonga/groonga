.. -*- rst -*-

.. groonga-command
.. database: tutorial-match-columns

match_columns parameter
=======================

Full-text search against multiple columns
-----------------------------------------

Groonga supports full-text search against multiple columns. Let's consider blog site. Usually, blog site has a table which contains title column and content column. How do you search the blog entry which contains specified keywords in title or content?

In such a case, there are two ways to create indexes. One way is creating column index against each column. The other way is creating one column index against multiple columns. Either way, Groonga supports similar full-text search syntax.

.. _creating-column-index-against-each-column:

Creating column index against each column
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here is the example which create column index against each column.

First, create ``Blog1`` table, add ``title`` column which stores title string, ``message`` column which stores content of blog entry.

Then create ``IndexBlog1`` table for column indexes, add ``index_title`` column for ``title`` column, ``index_message`` column for ``message`` column.

.. groonga-command
.. include:: ../example/tutorial/match_columns-1.log
.. table_create --name Blog1 --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Blog1 --name title --flags COLUMN_SCALAR --type ShortText
.. column_create --table Blog1 --name message --flags COLUMN_SCALAR --type ShortText
.. table_create --name IndexBlog1 --flags TABLE_PAT_KEY --key_type ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create --table IndexBlog1 --name index_title --flags COLUMN_INDEX|WITH_POSITION --type Blog1 --source title
.. column_create --table IndexBlog1 --name index_message --flags COLUMN_INDEX|WITH_POSITION --type Blog1 --source message
.. load --table Blog1
.. [
.. {"_key":"grn1","title":"Groonga test","message":"Groonga message"},
.. {"_key":"grn2","title":"baseball result","message":"rakutan eggs 4 - 4 Groonga moritars"},
.. {"_key":"grn3","title":"Groonga message","message":"none"}
.. ]

``match_columns`` option of ``select`` command accepts multiple columns as search target.
Specify query string to ``query`` option. Then you can do full-text search title and content of blog entries.

Let's try to search blog entries.

.. groonga-command
.. include:: ../example/tutorial/match_columns-2.log
.. select --table Blog1 --match_columns title||message --query groonga
.. select --table Blog1 --match_columns title||message --query message
.. select --table Blog1 --match_columns title --query message

Creating one column index against multiple columns
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Groonga also supports one column index against multiple columns.

The difference for previous example is only one column index exists. Thus, There is one common column index against title and message column.

Even though same column index is used, Groonga supports to search against title column only, message column only and title or message column.

.. groonga-command
.. include:: ../example/tutorial/match_columns-3.log
.. table_create --name Blog2 --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Blog2 --name title --flags COLUMN_SCALAR --type ShortText
.. column_create --table Blog2 --name message --flags COLUMN_SCALAR --type ShortText
.. table_create --name IndexBlog2 --flags TABLE_PAT_KEY --key_type ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create --table IndexBlog2 --name index_blog --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Blog2 --source title,message
.. load --table Blog2
.. [
.. {"_key":"grn1","title":"Groonga test","message":"Groonga message"},
.. {"_key":"grn2","title":"baseball result","message":"rakutan eggs 4 - 4 Groonga moritars"},
.. {"_key":"grn3","title":"Groonga message","message":"none"}
.. ]

Let's search same query in previous section. You can get same search results.

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


.. _full-text-search-with-specific-index-name:

Full text search with specific index name
-----------------------------------------

Groonga also supports full text search with specific index name.

In this section, you learn how to use specific index column efficiently.

Here is the concrete example about specific index name.

.. groonga-command
.. include:: ../example/tutorial/match_columns-specific-index-schema.log
.. table_create Entries TABLE_HASH_KEY ShortText
.. column_create Entries title COLUMN_SCALAR ShortText
.. column_create Entries body COLUMN_SCALAR ShortText
.. load --table Entries
.. [
.. {"_key": "http://example.com/entry1", "title":"Hello Groonga.", "body":"This is my first entry."},
.. {"_key": "http://example.com/entry2", "title":"Hello world.", "body":"I love Groonga!"},
.. {"_key": "http://example.com/entry3", "title":"Hello Mroonga, bye Groonga.", "body":"I use Mroonga."},
.. {"_key": "http://example.com/entry4", "title":"Say, Hello Groonga!", "body":"I'm back."}
.. ]
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Terms entries_title COLUMN_INDEX|WITH_POSITION Entries title
.. column_create Terms entries_body COLUMN_INDEX|WITH_POSITION Entries body
.. column_create Terms entries_whole COLUMN_INDEX|WITH_POSITION|WITH_SECTION Entries title,body

The table which stores entries has columns for title and body.
And the terms table has index columns for title and body to entries table.

There are three index columns in terms table.

* entries_title: index column for title
* entries_body: index column for body
* entries_whole: index column for title and body

If you specify index column which is related to specific data column, related data column is searched with that index implicitly.

For example, if you want to search title or body only, specify ``Terms.entries_title`` or ``Terms.entries_body`` index column.

.. groonga-command
.. include:: ../example/tutorial/match_columns-specific-title-index.log
.. select --table Entries --output_columns title --match_columns Terms.entries_title --query "Groonga"

This example uses ``Terms.entries_title`` as index, then search "Groonga" against title data column.

.. groonga-command
.. include:: ../example/tutorial/match_columns-specific-body-index.log
.. select --table Entries --output_columns body --match_columns Terms.entries_body --query "Groonga"

This example uses ``Terms.entries_body`` as index, then search "Groonga" against body data column.

If you specify multiple index column which is related to specific data columns, you can also specify data column name as suffix. It means that "Use specific index and search specific data column explicitly".

For example, if you want to search title or body only with ``entries_whole`` index, specify ``Terms.entries_whole.title`` or ``Terms.entries_whole.body``. It uses ``Terms.entries_whole`` index and search ``title`` column or ``body`` column explicitly.

.. groonga-command
.. include:: ../example/tutorial/match_columns-specific-whole-with-title.log
.. select --table Entries --output_columns title --match_columns Terms.entries_whole.title --query "Groonga"

This example uses ``Terms.entries_whole`` as index, then search "Groonga" against title data column.

.. groonga-command
.. include:: ../example/tutorial/match_columns-specific-whole-with-body.log
.. select --table Entries --output_columns body --match_columns Terms.entries_whole.body --query "Groonga"

This example uses ``Terms.entries_whole`` as index, then search "Groonga" against body data column.

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
.. table_create Lexicon TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Lexicon articles_content COLUMN_INDEX|WITH_POSITION Articles content
.. column_create Lexicon comments_content COLUMN_INDEX|WITH_POSITION Comments content
.. column_create Comments article COLUMN_INDEX Articles comment

Here is the sample data.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-data.log
.. load --table Comments
.. [
.. {"_key": 1, "content": "I'm using Groonga too!"},
.. {"_key": 2, "content": "I'm using Groonga and Mroonga!"},
.. {"_key": 3, "content": "I'm using Mroonga too!"}
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

You need to concatenate comment column of Articles table and content column of Comments table with period( ``.`` ) as ``--match_columns`` arguments.

At first, this query execute fulltext search from content of Comments table, then fetch the records of Articles table which refers to already searched records of Comments table.
(Because of this, if you comment out the query which creates index column ``article`` of Comments table, you can't get intended search results.)

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
.. table_create Lexicon2 TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
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
.. {"_key": 1, "content": "I'm using Rroonga too!"},
.. {"_key": 2, "content": "I'm using Groonga and Mroonga and Rroonga!"},
.. {"_key": 3, "content": "I'm using Nroonga too!"}
.. ]
.. load --table Comments2
.. [
.. {"_key": 1, "content": "I'm using Groonga too!", "comment": 1},
.. {"_key": 2, "content": "I'm using Groonga and Mroonga!", "comment": 2},
.. {"_key": 3, "content": "I'm using Mroonga too!"}
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

The first query searches ``mroonga`` from ``Comments2`` table, the second one searches ``mroonga`` from ``Replies2`` and ``Comments2`` table by using reference column index.

.. groonga-command
.. include:: ../example/tutorial/match_columns-nested-index-select-with-three-relationship.log
.. select Articles2 --match_columns comment.content --query mroonga --output_columns "_id, _score, *"
.. select Articles2 --match_columns comment.comment.content --query mroonga --output_columns "_id, _score, *"

As a result, the first query matches two article because of ``Comments2`` table has two records which contains ``mroonga`` as keyword.

On the other hand, the second one matches one article only because of ``Replies2`` table has only one record which contains ``mroonga`` as keyword, and there is one record which contains same keyword and refers to the record in ``Comments2`` table.

Indexes with Weight
-------------------

If index columns are created for data columns, you can search by indexes with weight.
For example, let's try to search blog entries by indexes with weight which contains ``Groonga`` as important keyword in ``Blog1`` table.
Generally speaking, an important keyword tend to be included in blog title, so if ``title`` column contains ``Groonga``, its score ( ``_score`` ) must be raised in contrast to ``message`` column. The indexes with weight is used for such a purpose.

Here is the example which search blog entries with ``Groonga`` as important keyword in ``title`` or ``message`` columns.

The sample schema and data is same as :ref:`creating-column-index-against-each-column`.

.. groonga-command
.. include:: ../example/tutorial/match_columns-indexes-with-weight.log
.. select --table Blog1 --match_columns 'IndexBlog1.index_title * 10 || IndexBlog1.index_message' --query 'Groonga' --output_columns "_id, _score, *"

In above query, ``'IndexBlog1.index_title * 10 || IndexBlog1.index_message'`` is specified for ``--match_columns``.
It means that if ``title`` column (search ``title`` column using ``IndexBlog1.index_title`` index) matches to ``Groonga``, its weight is multiplied to 10 and if ``message`` column (search ``message`` column using ``IndexBlog1.index_message`` index) matches to ``Groonga``,
its weight is 1 (default). If ``Groonga`` matches to ``title`` and ``message``, its weight is 11 (10 + 1) in this case.

As a result, ``Groonga test`` blog entry is listed in first.
