.. -*- rst -*-

.. groonga-command
.. database: tutorial

Basic operations
================

A Groonga package provides a C library (libgroonga) and a command line tool (groonga). This tutorial explains how to use the command line tool, with which you can create/operate databases, start a server, establish a connection with a server, etc.

Create a database
-----------------

The first step to using Groonga is to create a new database. The following shows how to do it.

Form::

  groonga -n DB_PATH

The ``-n`` option specifies to create a new database and DB_PATH specifies the path of the new database. Actually, a database consists of a series of files and DB_PATH specifies the file which will be the entrance to the new database. DB_PATH also specifies the path prefix for other files. Note that database creation fails if DB_PATH points to an existing file (For example, ``db open failed (DB_PATH): syscall error 'DB_PATH' (File exists)``. You can operate an existing database in a way that is in the next chapter).

This command creates a new database and then enters into interactive mode in which Groonga prompts you to enter commands for operating that database. You can terminate this mode with Ctrl-d.

.. groonga-command
.. include:: ../example/tutorial/introduction-1.log
.. .. % groonga -n /tmp/groonga-databases/introduction.db

After this database creation, you can find a series of files in /tmp/groonga-databases.

Operate a database
------------------

The following shows how to operate an existing database.

Form::

  groonga DB_PATH [COMMAND]

DB_PATH specifies the path of a target database. This command fails if the specified database does not exist.

If COMMAND is specified, Groonga executes COMMAND and returns the result. Otherwise, Groonga starts in interactive mode that reads commands from the standard input and executes them one by one. This tutorial focuses on the interactive mode.

Let's see the status of a Groonga process by using a :doc:`/reference/commands/status` command.

.. groonga-command
.. include:: ../example/tutorial/introduction-2.log
.. .. % groonga /tmp/groonga-databases/introduction.db
.. status

As shown in the above example, a command returns a JSON array. The first element contains an error code, execution time, etc. The second element is the result of an operation.

.. note::

   You can format a JSON using additional tools. For example, `grnwrap <https://github.com/michisu/grnwrap>`_, `Grnline <https://github.com/yoshihara/grnline>`_, `jq <http://stedolan.github.io/jq/>`_ and so on.

Command format
--------------

Commands for operating a database accept arguments as follows::

 Form_1: COMMAND VALUE_1 VALUE_2 ..

 Form_2: COMMAND --NAME_1 VALUE_1 --NAME_2 VALUE_2 ..

In the first form, arguments must be passed in order. This kind of arguments are called positional arguments because the position of each argument determines its meaning.

In the second form, you can specify a parameter name with its value. So, the order of arguments is not defined. This kind of arguments are known as named parameters or keyword arguments.

If you want to specify a value which contains white-spaces or special characters, such as quotes and parentheses, please enclose the value with single-quotes or double-quotes.

For details, see also the paragraph of "command" in :doc:`/reference/executables/groonga`.

Basic commands
--------------

 :doc:`/reference/commands/status`
  shows status of a Groonga process.
 :doc:`/reference/commands/table_list`
  shows a list of tables in a database.
 :doc:`/reference/commands/column_list`
  shows a list of columns in a table.
 :doc:`/reference/commands/table_create`
  adds a table to a database.
 :doc:`/reference/commands/column_create`
  adds a column to a table.
 :doc:`/reference/commands/select`
  searches records from a table and shows the result.
 :doc:`/reference/commands/load`
  inserts records to a table.

.. _tutorial-introduction-create-table:

Create a table
--------------

A :doc:`/reference/commands/table_create` command creates a new table.

In most cases, a table has a primary key which must be specified with its data type and index type.

There are various data types such as integers, strings, etc. See also :doc:`/reference/types` for more details. The index type determines the search performance and the availability of prefix searches. The details will be described later.

Let's create a table. The following example creates a table with a primary key. The ``name`` parameter specifies the name of the table. The ``flags`` parameter specifies the index type for the primary key. The ``key_type`` parameter specifies the data type of the primary key.

.. groonga-command
.. include:: ../example/tutorial/introduction-3.log
.. table_create --name Site --flags TABLE_HASH_KEY --key_type ShortText

The second element of the result indicates that the operation succeeded.

View a table
------------

A :doc:`/reference/commands/select` command can enumerate records in a table.

.. groonga-command
.. include:: ../example/tutorial/introduction-4.log
.. select --table Site

When only a table name is specified with a ``table`` parameter, a :doc:`/reference/commands/select` command returns the first (at most) 10 records in the table. [0] in the result shows the number of records in the table. The next array is a list of columns. ["_id","Uint32"] is a column of UInt32, named _id. ["_key","ShortText"] is a column of ShortText, named _key.

The above two columns, _id and _key, are the necessary columns. The _id column stores IDs those are automatically allocated by Groonga. The _key column is associated with the primary key. You are not allowed to rename these columns.

Create a column
---------------

A :doc:`/reference/commands/column_create` command creates a new column.

Let's add a column. The following example adds a column to the Site table. The ``table`` parameter specifies the target table. The ``name`` parameter specifies the name of the column. The ``type`` parameter specifies the data type of the column.

.. groonga-command
.. include:: ../example/tutorial/introduction-5.log
.. column_create --table Site --name title --type ShortText
.. select --table Site

Load records
------------

A :doc:`/reference/commands/load` command loads JSON-formatted records into a table.

The following example loads nine records into the Site table.

.. groonga-command
.. include:: ../example/tutorial/introduction-6.log
.. load --table Site
.. [
.. {"_key":"http://example.org/","title":"This is test record 1!"},
.. {"_key":"http://example.net/","title":"test record 2."},
.. {"_key":"http://example.com/","title":"test test record three."},
.. {"_key":"http://example.net/afr","title":"test record four."},
.. {"_key":"http://example.org/aba","title":"test test test record five."},
.. {"_key":"http://example.com/rab","title":"test test test test record six."},
.. {"_key":"http://example.net/atv","title":"test test test record seven."},
.. {"_key":"http://example.org/gat","title":"test test record eight."},
.. {"_key":"http://example.com/vdw","title":"test test record nine."},
.. ]

The second element of the result indicates how many records were successfully loaded. In this case, all the records are successfully loaded.

Let's make sure that these records are correctly stored.

.. groonga-command
.. include:: ../example/tutorial/introduction-7.log
.. select --table Site

Get a record
------------

A :doc:`/reference/commands/select` command can search records in a table.

If a search condition is specified with a ``query`` parameter, a :doc:`/reference/commands/select` command searches records matching the search condition and returns the matched records.

Let's get a record having a specified record ID. The following example gets the first record in the Site table. More precisely, the ``query`` parameter specifies a record whose _id column stores 1.

.. groonga-command
.. include:: ../example/tutorial/introduction-8.log
.. select --table Site --query _id:1

Next, let's get a record having a specified key. The following example gets the record whose primary key is "http://example.org/". More precisely, the ``query`` parameter specifies a record whose _key column stores "http://example.org/".

.. groonga-command
.. include:: ../example/tutorial/introduction-9.log
.. select --table Site --query '_key:"http://example.org/"'

Create a lexicon table for full text search
-------------------------------------------

Let's go on to how to make full text search.

Groonga uses an inverted index to provide fast full text search. So, the first step is to create a lexicon table which stores an inverted index, also known as postings lists. The primary key of this table is associated with a vocabulary made up of index terms and each record stores postings lists for one index term.

The following shows a command which creates a lexicon table named Terms. The data type of its primary key is ShortText.

.. groonga-command
.. include:: ../example/tutorial/introduction-10.log
.. table_create --name Terms --flags TABLE_PAT_KEY --key_type ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto

The :doc:`/reference/commands/table_create` command takes many parameters but you don't need to understand all of them. Please skip the next paragraph if you are not interested in how it works.

The TABLE_PAT_KEY flag specifies to store index terms in a patricia trie. The ``default_tokenizer`` parameter specifies the method for tokenizing text. This example uses TokenBigram that is generally called N-gram.

The ``normalizer`` parameter specifies to normalize index terms.

Create an index column for full text search
-------------------------------------------

The second step is to create an index column, which allows you to search records from its associated column. That is to say this step specifies which column needs an index.

Let's create an index column. The following example creates an index column for a column in the Site table.

.. groonga-command
.. include:: ../example/tutorial/introduction-11.log
.. column_create --table Terms --name blog_title --flags COLUMN_INDEX|WITH_POSITION --type Site --source title

The ``table`` parameter specifies the index table and the ``name`` parameter specifies the index column. The ``type`` parameter specifies the target table and the ``source`` parameter specifies the target column. The COLUMN_INDEX flag specifies to create an index column and the WITH_POSITION flag specifies to create a full inverted index, which contains the positions of each index term. This combination, COLUMN_INDEX|WITH_POSITION, is recommended for the general purpose.

.. note::

   You can create a lexicon table and index columns before/during/after loading records. If a target column already has records, Groonga creates an inverted index in a static manner. In contrast, if you load records into an already indexed column, Groonga updates the inverted index in a dynamic manner.

Full text search
----------------

It's time. You can make full text search with a :doc:`/reference/commands/select` command.

A query for full text search is specified with a ``query`` parameter. The following example searches records whose "title" column contains "this". The '@' specifies to make full text search. Note that a lower case query matches upper case and capitalized terms in a record if NormalizerAuto was specified when creating a lexcon table.

.. groonga-command
.. include:: ../example/tutorial/introduction-12.log
.. select --table Site --query title:@this

In this example, the first record matches the query because its title contains "This", that is the capitalized form of the query.

A :doc:`/reference/commands/select` command accepts an optional parameter, named `match_columns`, that specifies the default target columns. This parameter is used if target columns are not specified in a query. [#]_

The combination of "`--match_columns` title" and "`--query` this" brings you the same result that "`--query` title:@this" does.

.. groonga-command
.. include:: ../example/tutorial/introduction-13.log
.. select --table Site --match_columns title --query this

Specify output columns
----------------------

An ``output_columns`` parameter of a :doc:`/reference/commands/select` command specifies columns to appear in the search result. If you want to specify more than one columns, please separate column names by commas (',').

.. groonga-command
.. include:: ../example/tutorial/introduction-14.log
.. select --table Site --output_columns _key,title,_score --query title:@test

This example specifies three output columns including the _score column, which stores the relevance score of each record.

Specify output ranges
---------------------

A :doc:`/reference/commands/select` command returns a part of its search result if ``offset`` and/or ``limit`` parameters are specified. These parameters are useful to paginate a search result, a widely-used interface which shows a search result on a page by page basis.

An ``offset`` parameter specifies the starting point and a ``limit`` parameter specifies the maximum number of records to be returned. If you need the first record in a search result, the offset parameter must be 0 or omitted.

.. groonga-command
.. include:: ../example/tutorial/introduction-15.log
.. select --table Site --offset 0 --limit 3
.. select --table Site --offset 3 --limit 3
.. select --table Site --offset 7 --limit 3

Sort a search result
--------------------

A :doc:`/reference/commands/select` command sorts its result when used with a ``sort_keys`` parameter.

A ``sort_keys`` parameter specifies a column as a sorting creteria. A search result is arranged in ascending order of the column values. If you want to sort a search result in reverse order, please add a leading hyphen ('-') to the column name in a parameter.

The following example shows records in the Site table in reverse order.

.. groonga-command
.. include:: ../example/tutorial/introduction-16.log
.. select --table Site --sort_keys -_id

The next example uses the _score column as the sorting criteria for ranking the search result. The result is sorted in relevance order.

.. groonga-command
.. include:: ../example/tutorial/introduction-17.log
.. select --table Site --query title:@test --output_columns _id,_score,title --sort_keys -_score

If you want to specify more than one columns, please separate column names by commas (','). In such a case, a search result is sorted in order of the values in the first column, and then records having the same values in the first column are sorted in order of the second column values.

.. groonga-command
.. include:: ../example/tutorial/introduction-18.log
.. select --table Site --query title:@test --output_columns _id,_score,title --sort_keys -_score,_id

.. rubric:: footnote

.. [#] Currently, a ``match_columns`` parameter is available iff there exists an inverted index for full text search. A ``match_columns`` parameter for a regular column is not supported.
