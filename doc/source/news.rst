.. -*- rst -*-

:orphan:

News
====

.. _release-12-0-8:

Release 12.0.8 - 2022-09-29
---------------------------

Improvements
------------

* Added a new function ``escalate()``.

  We changed specification of the ``escalate()`` function as below.
  
  * Only use result sets inside ``escalate()`` for threshold.

    * Don't use the current result set out of ``escalate()``.
  
  * Don't require the threshold for the first condition. (e.g. ``escalate(CONDITION1, THRESHOLD2, CONDITION2, ...)``)
  * Don't allow empty arguments call. The first condition is required.
  * Always execute the first condition.

* [:doc:`install/cmake`] Added a document about how to build Groonga with CMake.

* [:doc:`install/others`] Added descriptions about how to enable/disable Apache Arrow support when building with GNU Autotools.

* [:doc:`reference/commands/select`] Added a document about ``drilldowns.table``.

* [:doc:`contribution/documentation/i18n`] Updated the translation procedure.

Fixes
-----

* Fixed a bug that Groonga could return incorrect results when we use :doc:`reference/normalizers/normalizer_table`
  and it contains a non-idempotent (results can be changed when executed repeatedly) definition.
  
  This was caused by that we normalized a search value multiple times: after the value was input and after the value was tokenized.

  Here is a example.

  .. code-block::

     table_create ColumnNormalizations TABLE_NO_KEY
     column_create ColumnNormalizations target_column COLUMN_SCALAR ShortText
     column_create ColumnNormalizations normalized COLUMN_SCALAR ShortText

     load --table ColumnNormalizations
     [
     {"target_column": "a", "normalized": "b"},
     {"target_column": "b", "normalized": "c"}
     ]

     table_create Targets TABLE_PAT_KEY ShortText
     column_create Targets column_normalizations_target_column COLUMN_INDEX \
       ColumnNormalizations target_column

     table_create Memos TABLE_NO_KEY
     column_create Memos content COLUMN_SCALAR ShortText

     load --table Memos
     [
     {"content":"a"},
     {"content":"c"},
     ]

     table_create \
       Terms \
       TABLE_PAT_KEY \
       ShortText \
       --default_tokenizer 'TokenNgram("unify_alphabet", false, \
                                       "report_source_location", true)' \
       --normalizers 'NormalizerTable("normalized", \
                                     "ColumnNormalizations.normalized", \
                                     "target", \
                                     "target_column")'

     column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content

     select Memos --query content:@a
     [[0,1664781132.892326,0.03527212142944336],[[[1],[["_id","UInt32"],["content","ShortText"]],[2,"c"]]]]

  The expected result of ``select Memos --query content:@a`` is ``a``, but Groonga returned ``c`` as a result.
  This was because we normalized the input ``a`` to ``b`` by definitions of ``ColumnNormalizations``, and after that, we normalized the normalized ``b``
  again and it was normalized to ``c``. As a result, the input ``a`` was converted to ``c`` and matched to ``{"content":"c"}`` of the ``Memos`` table.

.. _release-12-0-7:

Release 12.0.7 - 2022-08-29
---------------------------

Improvements
------------

* Added a new function ``escalate()``. (experimental)

  The ``escalate()`` function is similar to the existing match escalation ( :doc:`spec/search` ).
  We can use this function for any conditions. (The existing match escalation is just for one full text search by invert index.)

  The ``escalate()`` function is useful when we want to limit the number of results of a search.
  Even if we use ``--limit``, we can limit the number of results of a search. However, ``--limit`` is evaluated after evaluating all conditions in our query.
  The ``escalate()`` function finish the evaluation of conditions at that point when the result set has greater than ``THRESHOLD`` records. In other words, The ``escalate()`` function may reduce the number of evaluating conditions.

  The syntax of the ``escalate()`` function as below::

    escalate(THRESHOLD_1, CONDITION_1,
             THRESHOLD_2, CONDITION_2,
             ...,
             THRESHOLD_N, CONDITION_N)

  ``THRESHOLD_N`` is a positive number such as 0 and 29.
  ``CONDITION_N`` is a string that uses :doc:`reference/grn_expr/script_syntax` such as ``number_column > 29``.

  If the current result set has less than or equal to ``THRESHOLD_1`` records, the corresponding ``CONDITION_1`` is executed.
  Similarly, if the next result set has less than or equal to ``THRESHOLD_2`` records, the corresponding ``CONDITION_2`` is executed.
  If the next result set has greater than ``THRESHOLD_3`` records, the ``escalate()`` function is finished.

  If all ``CONDITION`` s are executed, ``escalate(THRESHOLD_1, CONDITION_1, ..., THRESHOLD_N, CONDITION_N)`` is same as ``CONDITION_1 || ... || CONDITION_N``.

  The ``escalate()`` function can be worked with logical operators such as ``&&`` and ``&!`` ::

    number_column > 10 && escalate(THRESHOLD_1, CONDITION_1,
                                   ...,
                                   THRESHOLD_N, CONDITION_N)
    number_column > 10 &! escalate(THRESHOLD_1, CONDITION_1,
                                   ...,
                                   THRESHOLD_N, CONDITION_N)

  They are same as ``number_column > 10 && (CONDITION_1 || ... || CONDITION_N)`` and ``number_column > 10 &! (CONDITION_1 || ... || CONDITION_N)`` .

  However, these behaviors may be changed because they may not be useful.

* [httpd] Updated bundled nginx to 1.23.1.

* [:doc:`reference/commands/select`] Add a document for the ``--n_workers`` option.

Fixes
-----

* Fixed a bug Groonga's response may be slow when we execute the ``request_cancel`` while executing a OR search.

  When the number of results of the OR search is many and a query has many OR conditions, Groonga may response slow with the "request_cancel" command.

.. _release-12-0-6:

Release 12.0.6 - 2022-08-04
---------------------------

Improvements
------------

* Added new Munin plugins for groonga-delta.

  We can monitoring the following items by plugins for groonga-delta.

    * Whether ``groonga-delta-import`` can import or not ``.grn`` file on local storage.
    * Whether ``groonga-delta-import`` can import or not difference data of MySQL.
    * Whether ``groonga-delta-apply`` can apply imported data or not.
    * The total size of applying data.

* [:doc:`reference/commands/column_copy`] Added support for weight vector.

  We can copy the value of weight vector by ``column_copy`` as below.

  .. code-block::

     table_create Tags TABLE_HASH_KEY ShortText
     [[0,0.0,0.0],true]
     table_create CopyFloat32Value TABLE_HASH_KEY ShortText
     [[0,0.0,0.0],true]
     column_create CopyFloat32Value source_tags COLUMN_VECTOR|WITH_WEIGHT|WEIGHT_FLOAT32 Tags
     [[0,0.0,0.0],true]
     column_create CopyFloat32Value destination_tags COLUMN_VECTOR|WITH_WEIGHT|WEIGHT_FLOAT32 Tags
     [[0,0.0,0.0],true]
     load --table CopyFloat32Value
     [
     {
       "_key": "Groonga is fast!!!",
       "source_tags": {
         "Groonga": 2.8,
         "full text search": 1.5
       }
     }
     ]
     [[0,0.0,0.0],1]
     column_copy CopyFloat32Value source_tags CopyFloat32Value destination_tags
     [[0,0.0,0.0],true]
     select CopyFloat32Value
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "_key",
               "ShortText"
             ],
             [
               "destination_tags",
               "Tags"
             ],
             [
               "source_tags",
               "Tags"
             ]
           ],
           [
             1,
             "Groonga is fast!!!",
             {
               "Groonga": 2.8,
               "full text search": 1.5
             },
             {
               "Groonga": 2.8,
               "full text search": 1.5
             }
           ]
         ]
       ]
     ]

* [:doc:`/install/ubuntu`] Dropped support for Ubuntu 21.10 (Impish Indri).

  Because Ubuntu 21.10 reached EOL in July 2022.

* [:doc:`/install/debian`] Dropped Debian 10 (buster) support.

  Because Debian 10 reaches EOL in August 2022.

Fixes
-----

* Fixed a bug that Groonga may crash when we execute drilldown in a parallel by ``n_workers`` option.

* [:doc:`reference/commands/select`] Fixed a bug that the syntax error occurred when we specify a very long expression in ``--filter``.

  Because the max stack size for the expression of ``--filter`` was 100 until now.

.. _release-12-0-5:

Release 12.0.5 - 2022-06-29
---------------------------

Improvements
------------

* [:doc:`reference/commands/select`] Improved a little bit of performance for prefix search by search escalation.

* [:doc:`reference/commands/select`] Added support for specifying a reference vector column with weight in ``drilldowns[LABEL]._key``. [GitHub#1366][Patched by naoa]

  If we specified a reference vector column with weight in drilldown's key, Groonga had returned incorrect results until now.

  For example, the following tag search had returned incorrect results until now.

  .. code-block::

     table_create Tags TABLE_PAT_KEY ShortText

     table_create Memos TABLE_HASH_KEY ShortText
     column_create Memos tags COLUMN_VECTOR|WITH_WEIGHT Tags
     column_create Memos date COLUMN_SCALAR Time

     load --table Memos
     [
     {"_key": "Groonga is fast!", "tags": {"full-text-search": 100}, "date": "2014-11-16 00:00:00"},
     {"_key": "Mroonga is fast!", "tags": {"mysql": 100, "full-text-search": 80}, "date": "2014-11-16 00:00:00"},
     {"_key": "Groonga sticker!", "tags": {"full-text-search": 100, "sticker": 10}, "date": "2014-11-16 00:00:00"},
     {"_key": "Rroonga is fast!", "tags": {"full-text-search": 100, "ruby": 20}, "date": "2014-11-17 00:00:00"},
     {"_key": "Groonga is good!", "tags": {"full-text-search": 100}, "date": "2014-11-17 00:00:00"}
     ]

     select Memos \
       --drilldowns[tags].keys tags \
       --drilldowns[tags].output_columns _key,_nsubrecs
     [
       [
         0,
         1656480220.591901,
         0.0005342960357666016
       ],
       [
         [
           [
             5
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "_key",
               "ShortText"
             ],
             [
               "date",
               "Time"
             ],
             [
               "tags",
               "Tags"
             ]
           ],
           [
             1,
             "Groonga is fast!",
             1416063600.0,
             {"full-text-search":100}
           ],
           [
             2,
             "Mroonga is fast!",
             1416063600.0,
             {"mysql":100,"full-text-search":80}
           ],
           [
             3,
             "Groonga sticker!",
             1416063600.0,
             {"full-text-search":100,"sticker":10}
           ],
           [
             4,
             "Rroonga is fast!",
             1416150000.0,
             {"full-text-search":100,"ruby":20}
           ],
           [
             5,
             "Groonga is good!",
             1416150000.0,
             {"full-text-search":100}
           ]
         ],
         {
           "tags": [
             [
               8
             ],
             [
               [
                 "_key",
                 "ShortText"
               ],
               [
                 "_nsubrecs",
                 "Int32"
               ]
             ],
             [
               "full-text-search",
               5
             ],
             [
               "f",
               5
             ],
             [
               "mysql",
               1
             ],
             [
               "f",
               1
             ],
             [
               "sticker",
               1
             ],
             [
               "f",
               1
             ],
             [
               "ruby",
               1
             ],
             [
               "f",
               1
             ]
           ]
         }
       ]

  The above query returns correct results as below since this release.

  .. code-block::

     select Memos   --drilldowns[tags].keys tags   --drilldowns[tags].output_columns _key,_nsubrecs
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             5
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "_key",
               "ShortText"
             ],
             [
               "date",
               "Time"
             ],
             [
               "tags",
               "Tags"
             ]
           ],
           [
             1,
             "Groonga is fast!",
             1416063600.0,
             {
               "full-text-search": 100
             }
           ],
           [
             2,
             "Mroonga is fast!",
             1416063600.0,
             {
               "mysql": 100,
               "full-text-search": 80
             }
           ],
           [
             3,
             "Groonga sticker!",
             1416063600.0,
             {
               "full-text-search": 100,
               "sticker": 10
             }
           ],
           [
             4,
             "Rroonga is fast!",
             1416150000.0,
             {
               "full-text-search": 100,
               "ruby": 20
             }
           ],
           [
             5,
             "Groonga is good!",
             1416150000.0,
             {
               "full-text-search": 100
             }
           ]
         ],
         {
           "tags": [
             [
               4
             ],
             [
               [
                 "_key",
                 "ShortText"
               ],
               [
                 "_nsubrecs",
                 "Int32"
               ]
             ],
             [
               "full-text-search",
               5
             ],
             [
               "mysql",
               1
             ],
             [
               "sticker",
               1
             ],
             [
               "ruby",
               1
             ]
           ]
         }
       ]
     ]

* [:doc:`reference/commands/select`] Added support for doing drilldown with a reference vector with weight even if we use ``query`` or ``filter``, or ``post_filter``. [GitHub#1367][Patched by naoa]

  If we specified a reference vector column with weight in drilldown's key when we use ``query`` or ``filter``, or ``post_filter``, Groonga had returned incorrect results or errors until now.

  For example, the following query had been erred until now.

  .. code-block::

     table_create Tags TABLE_PAT_KEY ShortText

     table_create Memos TABLE_HASH_KEY ShortText
     column_create Memos tags COLUMN_VECTOR|WITH_WEIGHT Tags
     column_create Memos date COLUMN_SCALAR Time

     load --table Memos
     [
     {"_key": "Groonga is fast!", "tags": {"full-text-search": 100}, "date": "2014-11-16 00:00:00"},
     {"_key": "Mroonga is fast!", "tags": {"mysql": 100, "full-text-search": 80}, "date": "2014-11-16 00:00:00"},
     {"_key": "Groonga sticker!", "tags": {"full-text-search": 100, "sticker": 10}, "date": "2014-11-16 00:00:00"},
     {"_key": "Rroonga is fast!", "tags": {"full-text-search": 100, "ruby": 20}, "date": "2014-11-17 00:00:00"},
     {"_key": "Groonga is good!", "tags": {"full-text-search": 100}, "date": "2014-11-17 00:00:00"}
     ]

     select Memos \
       --filter true \
       --post_filter true \
       --drilldowns[tags].keys tags \
       --drilldowns[tags].output_columns _key,_nsubrecs
     [
       [
         -22,
         1656473820.734894,
         0.03771400451660156,
         "[hash][add][           ] key size unmatch",
         [
           [
             "grn_hash_add",
             "hash.c",
             3405
           ]
         ]
       ],
       [
         [
         ]
       ]
     ]

  The above query returns correct results as below since this release.

  .. code-block::

     select Memos \
       --filter true \
       --post_filter true \
       --drilldowns[tags].keys tags \
       --drilldowns[tags].output_columns _key,_nsubrecs
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             5
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "_key",
               "ShortText"
             ],
             [
               "date",
               "Time"
             ],
             [
               "tags",
               "Tags"
             ]
           ],
           [
             1,
             "Groonga is fast!",
             1416063600.0,
             {
               "full-text-search": 100
             }
           ],
           [
             2,
             "Mroonga is fast!",
             1416063600.0,
             {
               "mysql": 100,
               "full-text-search": 80
             }
           ],
           [
             3,
             "Groonga sticker!",
             1416063600.0,
             {
               "full-text-search": 100,
               "sticker": 10
             }
           ],
           [
             4,
             "Rroonga is fast!",
             1416150000.0,
             {
               "full-text-search": 100,
               "ruby": 20
             }
           ],
           [
             5,
             "Groonga is good!",
             1416150000.0,
             {
               "full-text-search": 100
             }
           ]
         ],
         {
           "tags": [
             [
               4
             ],
             [
               [
                 "_key",
                 "ShortText"
               ],
               [
                 "_nsubrecs",
                 "Int32"
               ]
             ],
             [
               "full-text-search",
               5
             ],
             [
               "mysql",
               1
             ],
             [
               "sticker",
               1
             ],
             [
               "ruby",
               1
             ]
           ]
         }
       ]
     ]

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* naoa

.. _release-12-0-4:

Release 12.0.4 - 2022-06-06
---------------------------

Improvements
------------

* [:doc:`/install/ubuntu`] Added support for Ubuntu 22.04 (Jammy Jellyfish).

* We don't provide `groonga-benchmark`.

  Because nobody will not use it and we can't maintain it.

* [:doc:`reference/commands/status`] Added a new item ``memory_map_size``.

  We can get the total memory map size in bytes of Groonga with ``status`` command.

  .. code-block::

      status
      [
        [
          0,
          1654237168.304533,
          0.0001480579376220703
        ],
        {
          (omitted)
          "memory_map_size": 70098944
        }
      ]

  For example, in Windows, if Groonga uses up physical memory and swap area, Groonga can't more mapping memory than that.
  Therefore, we can control properly memory map size by monitoring this value even if the environment does have not enough memory.

Fixes
-----

* Fixed a bug Groonga's response may be slow when we execute ``request_cancel`` while executing a search.

  Especially, when the number of records of the search target is many, Groonga's response may be very slow.

* Fixed a bug that string list can't be casted to int32 vector.

  For example, the following cast had failed.

  * ["10", "100"] -> [10, 100]

  This bug only occurs when we specify ``apache-arrow`` into ``input_type`` as the argument of ``load``.
  This bug occurs in Groonga 12.0.2 or later.

* Fixed a bug that Groonga Munin Plugins do not work on AlmaLinux 8 and CentOS 7.

.. _release-12-0-3:

Release 12.0.3 - 2022-04-29
---------------------------

Improvements
------------

* [:doc:`reference/commands/logical_count`] Improved memory usage while ``logical_count`` executed.

  Up to now, Groonga had been keeping objects(objects are tables and columns and indexes, and so on) and temporary tables that were allocated while ``logical_count`` executed until the execution of ``logical_count`` finished.

  Groonga reduces reference immediately after processing a shard by this feature.
  Therefore, Groonga can release memory while ``logical_count`` executed.
  The usage of memory of Groonga may reduce because of these reasons.

  This improvement is only valid for the reference count mode.
  We can valid the reference count mode by setting ``GRN_ENABLE_REFERENCE_COUNT=yes``.

  In addition, Groonga releases temporary tables dynamically while ``logical_count`` is executed by this feature.
  Therefore, the usage of memory of Groonga reduces.
  This improvement is valid even if we don't set the reference count mode.

* [:doc:`/reference/commands/dump`] Added support for ``MISSING_IGNORE/MISSING_NIL``.

  If columns had ``MISSING_IGNORE/MISSING_NIL``, the dump of these columns had failed until now.
  ``dump`` command supports these columns since this release.

* [:doc:`reference/functions/snippet`],[:doc:`reference/functions/snippet_html`] Added support for text vector as input. [groonga-dev,04956][Reported by shinonon]

  For example, we can extract snippets of target text around search keywords against vector in JSON data as below.

  .. code-block::

     table_create Entries TABLE_NO_KEY
     column_create Entries title COLUMN_SCALAR ShortText
     column_create Entries contents COLUMN_VECTOR ShortText

     table_create Tokens TABLE_PAT_KEY ShortText   --default_tokenizer TokenNgram   --normalizer NormalizerNFKC130
     column_create Tokens entries_title COLUMN_INDEX|WITH_POSITION Entries title
     column_create Tokens entries_contents COLUMN_INDEX|WITH_SECTION|WITH_POSITION   Entries contents

     load --table Entries
     [
     {
       "title": "Groonga and MySQL",
       "contents": [
         "Groonga is a full text search engine",
         "MySQL is a RDBMS",
         "Mroonga is a MySQL storage engine based on Groonga"
       ]
     }
     ]

     select Entries\
       --output_columns 'snippet_html(contents), contents'\
       --match_columns 'title'\
       --query Groonga
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "snippet_html",
               null
             ],
             [
               "contents",
               "ShortText"
             ]
           ],
           [
             [
               "<span class=\"keyword\">Groonga</span> is a full text search engine",
               "Mroonga is a MySQL storage engine based on <span class=\"keyword\">Groonga</span>"
             ],
             [
               "Groonga is a full text search engine",
               "MySQL is a RDBMS",
               "Mroonga is a MySQL storage engine based on Groonga"
             ]
           ]
         ]
       ]
     ]

  Until now, if we specified ``snippet*`` like ``--output_columns 'snippet_html(contents[1])``,
  we could extract snippets of target text around search keywords against the vector as below.
  However, we didn't know which we should output elements. Because we didn't know which element was hit on search.

  .. code-block::

     select Entries\
       --output_columns 'snippet_html(contents[0]), contents'\
       --match_columns 'title'\
       --query Groonga
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "snippet_html",
               null
             ],
             [
               "contents",
               "ShortText"
             ]
           ],
           [
             [
               "<span class=\"keyword\">Groonga</span> is a full text search engine"
             ],
             [
               "Groonga is a full text search engine",
               "MySQL is a RDBMS",
               "Mroonga is a MySQL storage engine based on Groonga"
             ]
           ]
         ]
       ]
     ]

* [``vector_join``] Added a new function ``vector_join()``.[groonga-dev,04956][Reported by shinonon]

  This function can concatenate each elements.
  We can specify delimiter in the second argument in this function.

  For example, we could execute ``snippet()`` and ``snippet_html()`` against vector that concatenate each elements as below.

  .. code-block::

     plugin_register functions/vector

     table_create Entries TABLE_NO_KEY
     column_create Entries title COLUMN_SCALAR ShortText
     column_create Entries contents COLUMN_VECTOR ShortText

     table_create Tokens TABLE_PAT_KEY ShortText   --default_tokenizer TokenNgram   --normalizer NormalizerNFKC130
     column_create Tokens entries_title COLUMN_INDEX|WITH_POSITION Entries title
     column_create Tokens entries_contents COLUMN_INDEX|WITH_SECTION|WITH_POSITION   Entries contents

     load --table Entries
     [
     {
       "title": "Groonga and MySQL",
       "contents": [
         "Groonga is a full text search engine",
         "MySQL is a RDBMS",
         "Mroonga is a MySQL storage engine based on Groonga"
       ]
     }
     ]

     select Entries\
       --output_columns 'snippet_html(vector_join(contents, "\n")), contents'\
       --match_columns 'title'\
       --query Groonga --output_pretty yes
     [
       [
         0,
         1650849001.524027,
         0.0003361701965332031
       ],
       [
         [
           [
             1
           ],
           [
             [
               "snippet_html",
               null
             ],
             [
               "contents",
               "ShortText"
             ]
           ],
           [
             [
               "<span class=\"keyword\">Groonga</span> is a full text search engine\nMySQL is a RDBMS\nMroonga is a MySQL storage engine based on <span class=\"keyword\">Groonga</span>"
             ],
             [
               "Groonga is a full text search engine","MySQL is a RDBMS","Mroonga is a MySQL storage engine based on Groonga"
             ]
           ]
         ]
       ]
     ]

* [:doc:`/reference/indexing`] Ignore too large a token like online index construction. [GitHub:pgroonga/pgroonga#209][Reported by Zhanzhao (Deo) Liang]

  Groonga doesn't occur error, but Groonga ignores too large a token when we execute static index construction.
  However, Groonga output warning in this case.

Fixes
-----

* Fixed a bug that we may be not able to add a key to a table of patricia trie.

  This bug occurs in the following conditon.

  * If a table of patricia trie already has a key.
  * If the additional key is 4096 bytes.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* shinonon
* Zhanzhao (Deo) Liang

.. _release-12-0-2:

Release 12.0.2 - 2022-03-29
---------------------------

Improvements
------------

* [:doc:`reference/commands/logical_range_filter`] Added support for reducing reference immediately after processing a shard.

  Groonga had reduced reference all shards when the finish of ``logical_range_filter`` until now.
  Groonga reduces reference immediately after processing a shard by this feature.
  The usage of memory may reduce while ``logical_range_filter`` executes by this feature.  

  This feature is only valid for the reference count mode.
  We can valid the reference count mode by setting ``GRN_ENABLE_REFERENCE_COUNT=yes``.

  Normally, Groonga keep objects(tables and column and index, and so on) that Groonga opened even once on memory.
  However, if we open many objects, Groonga uses much memory.
  In the reference count mode release objects that are not referenced anywhere from memory.
  The usage of memory of Groonga may reduce by this.

* We increased the stability of the feature of recovering on crashes.

  This feature is experimental and it is disabled by default.
  Therefore, the following improvements are no influence on ordinary users.

  * We fixed a bug that the index was broken when Groonga crashed.
  * We fixed a bug that might remain a lock.
  * We fixed a bug that Groonga crashed while it was recovering the crash.

* Improved performance for mmap if anonymous mmap available.[GitHub:MariaDB/server#1999][Suggested by David CARLIER]

  The performance of Groonga is improved a bit by this improvement.

* [:doc:`/reference/indexing`] Added support for the static index construction against the following types of columns.

  * The non-reference vector column with weight
  * The reference vector column with weight
  * The reference scalar column

  These columns have not supported the static index construction until now.
  Therefore, the time of making the index has longed even if we set the index against these columns after we loaded data into them.
  By this improvement, the time of making the index is short in this case.

* [:doc:`reference/commands/column_create`] Added new flags ``MISSING_*`` and ``INVALID_*``.

  We added the following new flags for ``column_create``.

    * ``MISSING_ADD``
    * ``MISSING_IGNORE``
    * ``MISSING_NIL``

    * ``INVALID_ERROR``
    * ``INVALID_WARN``
    * ``INVALID_IGNORE``

  Normally, if the data column is a reference data column and the nonexistent key is specified, a new record for the nonexistent key is newly created automatically.

  The behavior that Groonga adds the key automatically into the column of reference destination is useful in the search like tag search.
  Because Groonga adds data automatically when we load data.

  However, this behavior is inconvenient if we need the other data except for the key.
  Because a record that only has the key exists.

  We can change this behavior by using flags that are added in this release.

    * ``MISSING_ADD``: This is the default value. This is the same behavior as the current.

      If the data column is a reference data column and the nonexistent key is specified, a new record for the nonexistent key is newly created automatically.

    * ``MISSING_IGNORE``:

      If the data column is a reference data column and the nonexistent key is specified, the nonexistent key is ignored.
      If the reference data column is a scalar column, the value is ``GRN_ID_NIL``.
      If the reference data column is a vector column, the element is just ignored as below ::

        ["existent1", "nonexistent", "existent2"] ->
        ["existent1" "existent2"]

    * ``MISSING_NIL``:

      If the data column is a reference data column and the nonexistent key is specified, the nonexistent key in a scalar column and a vector column is treated as ``GRN_ID_NIL`` ::

        ["existent1", "nonexistent", "existent2"] ->
        ["existent1", "" (GRN_ID_NIL), "existent2"]


    * ``INVALID_ERROR``: This is the default value. This is the same behavior as the current except an error response of a vector column case.

      If we set the invalid value (e.g.: ``XXX`` for ``UInt8`` scalar column), the set operation is treated as an error.
      If the data column is a scalar column, ``load`` reports an error in log and response.
      If the data column is a vector column, ``load`` reports an error in log but doesn't report an error in response.
      This is an incompatible change.

    * ``INVALID_WARN``:

      If we set the invalid value (e.g.: ``XXX`` for ``UInt8`` scalar column), a warning message is logged and the set operation is ignored.
      If the target data column is a reference vector data column, ``MISSING_IGNORE`` and ``MISSING_NIL`` are used to determine the behavior.

    * ``INVALID_IGNORE``:

      If we set the invalid value (e.g.: ``XXX`` for ``UInt8`` scalar column), the set operation is ignored.
      If the target data column is a reference vector data column, ``MISSING_IGNORE`` and ``MISSING_NIL`` are used to determine the behavior.

* [:doc:`/reference/commands/dump`][:doc:`/reference/commands/column_list`] Added support for ``MISSING_*`` and ``INVALID_*`` flags.

  These commands doesn't show ``MISSING_ADD`` and ``INVALID_ERROR`` flags to keep backward compatibility.
  Because these flags show the default behavior.

* [:doc:`/reference/commands/schema`] Added support for ``MISSING_*`` and ``INVALID_*`` flags.

  ``MISSING_AND`` and ``INVALID_ERROR`` flags aren't shown in ``flags`` to keep backward compatibility.
  However, new ``missing`` and ``invalid`` keys are added to each column.

* We provided the package of Amazon Linux 2.

* [Windows] Dropped support for building with Visual Studio 2017.

  Because we could not use windows-2016 image on GitHub Actions.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* David CARLIER

.. _release-12-0-1:

Release 12.0.1 - 2022-02-28
---------------------------

Improvements
------------

* [:doc:`/reference/commands/query_expand`] Added a support for synonym group.

  Until now, We had to each defined a keyword and synonyms of the keyword as below when we use the synonym search.

  .. code-block::

     table_create Thesaurus TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
     # [[0, 1337566253.89858, 0.000355720520019531], true]
     column_create Thesaurus synonym COLUMN_VECTOR ShortText
     # [[0, 1337566253.89858, 0.000355720520019531], true]
     load --table Thesaurus
     [
     {"_key": "mroonga", "synonym": ["mroonga", "tritonn", "groonga mysql"]},
     {"_key": "groonga", "synonym": ["groonga", "senna"]}
     ]

  In the above case, if we search ``mroonga``, Groonga search ``mroonga OR tritonn OR "groonga mysql"`` as we intended.
  However, if we search ``tritonn``, Groonga search only ``tritonn``.
  If we want to search ``tritonn OR mroonga OR "groonga mysql"`` even if we search ``tritonn``, we need had added a definition as below.

  .. code-block::

     load --table Thesaurus
     [
     {"_key": "tritonn", "synonym": ["tritonn", "mroonga", "groonga mysql"]},
     ]

  In many cases, if we expand ``mroonga`` to ``mroonga OR tritonn OR "groonga mysql"``, we feel we want to expand ``tritonn`` and ``"groonga mysql"`` to ``mroonga OR tritonn OR "groonga mysql"``.
  However, until now, we had needed additional definitions in such a case.
  Therefore, if target keywords for synonyms are many, we are troublesome to define synonyms.
  Because we need to define many similar definitions.

  In addition, when we remove synonyms, we are troublesome because we need to execute remove against many records.

  We can make a group by deciding on a representative synonym record since this release.
  For example, the all following keywords are the "mroonga" group.

  .. code-block::

     load --table Synonyms
     [
       {"_key": "mroonga": "representative": "mroonga"}
     ]

     load --table Synonyms
     [
       {"_key": "tritonn": "representative": "mroonga"},
       {"_key": "groonga mysql": "representative": "mroonga"}
     ]

  In this case, ``mroonga`` is expanded to ``mroonga OR tritonn OR "groonga mysql"``.
  In addition, ``tritonn`` and ``"groonga mysql"`` are also expanded to ``mroonga OR tritonn OR "groonga mysql"``.

  When we want to remove synonyms, we execute just remove against a target record.
  For example, if we want to remove ``"groonga mysql"`` from synonyms, we just remove ``{"_key": "groonga mysql": "representative": "mroonga"}``.

* [:doc:`/reference/commands/query_expand`] Added a support for text vector and index.

  We can use text vector in a synonym group as below.

  .. code-block::

    table_create SynonymGroups TABLE_NO_KEY
    [[0,0.0,0.0],true]
    column_create SynonymGroups synonyms COLUMN_VECTOR ShortText
    [[0,0.0,0.0],true]
    table_create Synonyms TABLE_PAT_KEY ShortText
    [[0,0.0,0.0],true]
    column_create Synonyms group COLUMN_INDEX SynonymGroups synonyms
    [[0,0.0,0.0],true]
    load --table SynonymGroups
    [
    ["synonyms"],
    [["rroonga", "Ruby groonga"]],
    [["groonga", "rroonga", "mroonga"]]
    ]
    [[0,0.0,0.0],2]
    query_expand Synonyms.group "rroonga"
    [
      [
        0,
        0.0,
        0.0
      ],
      "((rroonga) OR (Ruby groonga) OR (groonga) OR (rroonga) OR (mroonga))"
    ]

* Added support for disabling a backtrace by the environment variable.

  We can disable output a backtrace by using ``GRN_BACK_TRACE_ENABLE``.
  If we set ``GRN_BACK_TRACE_ENABLE=no``, Groonga doesn't output a backtrace.

  Groonga output backtrace to a stack area. Therefore, Groonga may crash because Groonga uses up stack area depending on the OS.
  In such cases, we can avoid crashes by using ``GRN_BACK_TRACE_ENABLE=no``.

* [:doc:`reference/commands/select`] Improved performance for ``--slices``.

* [Windows] Added support for Visual Studio 2022.

* [:doc:`reference/commands/select`] Added support for specifing max intervals for each elements in near search.

  For example, we can specify max intervals for each phrase in a near phrase search.
  We make documentation for this feature in the future. Therefore, we will make more details later.   

* [:doc:`reference/executables/groonga-server-http`] We could use ``groonga-server-http`` even if Groonga of RPM packages.

Fixes
^^^^^

* [Windows] Fixed a crash bug when Groonga output backtrace.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

.. _release-12-0-0:

Release 12.0.0 - 2022-02-09
---------------------------

This is a major version up!
But It keeps backward compatibility. We can upgrade to 12.0.0 without rebuilding database.

First of all, we introduce the Summary of changes from Groonga 11.0.0 to 11.1.3.
Then, we introduce the main changes in 12.0.0.

Summary of changes from Groonga 11.0.0 to 11.1.3
------------------------------------------------

New Features and Improvements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* [:doc:`reference/functions/snippet`] Added support for using the keyword of 32 or more.

  We could not specify the keyword of 32 or more with snippet until now.
  However, we can specify the keyword of 32 or more by this improvement.

  We don't specify the keyword of 32 or more with snippet in normal use.
  However, if the keyword increments automatically by using such as ``query_expand``, the number of target keywords may be 32 or more.

  In this case, Groonga occurs an error until now. However, Groonga doesn't occur an error by this improvement.

  See :ref:`release 11.1.3 <release-11-1-3>` for details.

* [:doc:`reference/normalizers/normalizer_nfkc130`] Added a new option ``remove_symbol``.

  This option removes symbols (e.g. #, !, “, &, %, …) from the string that the target of normalizing.
  For example, this option useful when we prevent orthographical variants such as a title of song and name of artist, and a name of store.

  See :ref:`release 11.1.3 <release-11-1-3>` for details.

* [:doc:`/reference/commands/load`] Added support for ISO 8601 time format.

  ISO 8601 format is the format generally.
  Therefore ``load`` becomes easy to use by Groonga support the more standard format.

  See :ref:`release 11.1.0 <release-11-1-0>` for details.

* [:doc:`reference/functions/snippet`] Added a new option ``delimiter_regexp`` for detecting snippet delimiter with regular expression.

  This feature is useful in that we want to show by the sentence the result of the search.

  :doc:`reference/functions/snippet` extracts text around search keywords.
  We call the text that is extracted by :doc:`reference/functions/snippet` snippet.

  Normally, :doc:`reference/functions/snippet` () returns the text of 200 bytes around search keywords.
  However, :doc:`reference/functions/snippet` () gives no thought to a delimiter of sentences.
  The snippet may be composed of multi sentences.

  ``delimiter_regexp`` option is useful if we want to only extract the text of the same sentence as search keywords.
  For example, we can use ``\.\s*`` to extract only text in the target sentence.
  Note that you need to escape ``\`` in string.

  See :ref:`release 11.0.9 <release-11-0-9>` for details.

* [:doc:`reference/commands/cache_limit`] Groonga remove query cache when we execute ``cache_limit 0``.

  Groonga stores query cache to internally table.
  The maximum total size of keys of this table is 4GiB. Because this table is hash table.
  Therefore, If we execute many huge queries, Groonga may be unable to store query cache, because the maximum total size of keys may be over 4GiB.
  In such cases, We can clear the table for query cache by using ``cache_limit 0``, and Groonga can store query cache 

  We needed to restart Groonga to resolve this problem until now.
  However, We can resolve this problem if we just execute ``cache_limit 0`` by this improvement.

  See :ref:`release 11.0.6 <release-11-0-6>` for details.

* [:doc:`/reference/functions/between`] Added support for optimizing the order of evaluation of a conditional expression.

  We can use the optimization of  the order of evaluation of a conditional expression in ``between()`` by setting ``GRN_EXPR_OPTIMIZE=yes``.
  This optimization is effective with respect if ``between()`` narrow down records enough or ``between()`` can't narrow down few records.

* [:doc:`reference/log`] Added support for outputting to stdout and stderr.

  This feature is useful when we execute Groonga on Docker.
  Docker has the feature that records stdout and stderr in standard.
  Therefore, we don't need to login into the environment of Docker to get Groonga's log.

  See :ref:`release 11.0.4 <release-11-0-4>` for details.

* [:doc:`reference/functions/query`] Added support for ignoring ``TokenFilterStem`` and ``TokenFilterStopWord`` by the query.

  We are able to search without ``TokenFilterStem`` and ``TokenFilterStopWord`` in only a specific query.

  This feature is useful when we want to search for the same words exactly as a search keyword.
  Normally, Groonga gets better results with enable stemming and stopwords excepting.
  However, if we want to search words the same as a keyword of search exactly, These features are needless.

  Until now, If we want to search words the same as a keyword of search exactly,
  We needed to make the index of the exclusive use.
  By this improvement, we can search words the same as a keyword of search exactly
  without making the index of the exclusive use.

  See :ref:`release 11.0.3 <release-11-0-3>` for details.

* [:doc:`/reference/functions/string_slice`] Added a new function ``string_slice()``.

  ``string_slice()`` extracts a substring of a string of search results by position or regular expression.
  This function is useful if we want to edit search results.

  For example, this feature is useful in that we exclude tags from search results.

* [:doc:`reference/functions/query_parallel_or`] Added a new function for processing queries in parallel.

  ``query_parallel_or`` is similar to query but ``query_parallel_or`` processes query that has multiple OR conditions in parallel.
  We can increase in speed of the execution of many OR conditions by using this function.

  However, ``query_parallel_or`` alone uses multiple CPUs.
  Therefore, queries that are executing at the same time as the query that executes ``query_parallel_or`` may be slow.

* [:doc:`/reference/token_filters`] Added support for multiple token filters with options.

  We can use multiple token filters with options as the following example.

  .. code-block::

     --token_filters 'TokenFilterStopWord("column", "ignore"), TokenFilterNFKC130("unify_kana", true)'

* [:doc:`reference/commands/select`] Added support for ``--post_filter`` and ``--slices[].post_filter``.

  We can filter again after we execute ``--filter`` by using ``--post_filter`` and ``--slices[].post_filter``.
  The difference between ``--post_filter`` and ``--slices[].post_filter`` is response format.

  The response format of ``--post_filter`` same as the response of ``--filter``.
  The response format of ``--slices[].post_filter`` show the result of before and after ``--slices[].post_filter`` executed.

  Note that if we use ``--slices[].post_filter``, the format of the response is different from the normal ``select`` 's response.

Fixes
^^^^^

* Fixed a bug that the version up of Groonga failed Because the version up of arrow-libs on which Groonga depends.

  However, if arrow-libs update a major version, this problem reproduces.
  In this case, we will handle that by rebuilding the Groonga package.

  This bug only occurs AlmaLinux 8 and CentOS 7.

* [Windows] Fixed a resource leak when Groonga fail open a new file caused by out of memory.

* Fixed a bug that Groonga may not have returned a result of a search query if we sent many search queries when tokenizer, normalizer, or token_filters that support options were used.

* Fixed a bug that there is possible that index is corrupt when Groonga executes many additions, delete, and update information in it.

  This bug occurs when we only execute many delete information from index.
  However, it doesn’t occur when we only execute many additions information into index.

  See :ref:`release 11.0.0 <release-11-0-0>` for details.

Newly supported OSes
^^^^^^^^^^^^^^^^^^^^

* [:doc:`/install/almalinux`] Added support for AlmaLinux 8.

* [:doc:`/install/almalinux`] Added support for AlmaLinux 8 for ARM64.

* [:doc:`/install/debian`] Added support for Debian 11 (Bullseye).

* [:doc:`/install/debian`] Added support for Debian 11 (Bullseye) for ARM64 and Debian 10 (buster) for ARM64.

* [:doc:`/install/ubuntu`] Added support for Ubuntu 21.10 (Impish Indri).

Dropped support OSes
^^^^^^^^^^^^^^^^^^^^

* [:doc:`/install/centos`] Dropped support for CentOS 8.

* [:doc:`/install/ubuntu`] Dropped support for Ubuntu 21.04 (Hirsute Hippo).

* [:doc:`/install/ubuntu`] Dropped support for Ubuntu 20.10 (Groovy Gorilla).

* [:doc:`/install/ubuntu`] Dropped support for Ubuntu 16.04 LTS (Xenial Xerus).

* [:doc:`/install/windows`] Dropped support for the following packages of Windows version that we had cross-compiled by using MinGW on Linux.

  * groonga-x.x.x-x86.exe
  * groonga-x.x.x-x86.zip
  * groonga-x.x.x-x64.exe
  * groonga-x.x.x-x86.zip

Thanks
------

* naoa
* Anthony M. Cook
* MASUDA Kazuhiro
* poti
* Takashi Hashida
* higchi
* wi24rd
* Josep Sanz
* Keitaro YOSHIMURA
* shibanao4870

The main changes in 12.0.0 are as follows.

Improvements
------------

* [:doc:`reference/functions/sub_filter`] Added a new option ``pre_filter_threshold``.

  We can change the value of ``GRN_SUB_FILTER_PRE_FILTER_THRESHOLD`` by this option.
  If the number of records is less than ``GRN_SUB_FILTER_PRE_FILTER_THRESHOLD`` when Groonga executes ``sub_filter``, Groonga execute ``sub_filter`` against records that have been already narrowed down.  

  We can use -1 to always use this optimization.

* [index_column_have_source_record] Added a new function ``index_column_have_source_record()``.

  We can confirm whether a token that is existing in the index is included in any of the records that are registered in Groonga or not.

  Groonga does not remove a token even if the token become never used from records in Groonga by updating records.
  Therefore, for example, when we use the feature of autocomplete, Groonga may return a token that is not included in any of the records as candidates for search words.
  However, we can become that we don't return the needless token by using this function.

  Because this function can detect a token that is not included in any of the records.

* [:doc:`reference/normalizers/normalizer_nfkc130`] Added a new option ``strip``

  This option removes spaces from the start and the end as below.

  .. code-block::

     normalize \
     'NormalizerNFKC121("strip", true, \
                        "report_source_offset", true)' \
     "  hello world\t! \t " \
     WITH_CHECKS|WITH_TYPES
      [
        [
          0,
          0.0,
          0.0
        ],
        {
          "normalized": "hello world!",
          "types": [
            "alpha",
            "alpha",
            "alpha",
            "alpha",
            "alpha",
            "others",
            "alpha",
            "alpha",
            "alpha",
            "alpha",
            "alpha|blank",
            "symbol|blank"
          ],
          "checks": [
            3,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            2
          ],
          "offsets": [
            0,
            3,
            4,
            5,
            6,
            7,
            8,
            9,
            10,
            11,
            12,
            14
          ]
        }
      ]

* [:doc:`reference/commands/select`] Added new arguments ``drilldown_max_n_target_records`` and ``drilldown[${LABEL}].max_n_target_records``.

  We can specify the max number of records of the drilldown target table (filtered result) to use drilldown.
  If the number of filtered result is larger than the specified value, some records in filtered result aren't used for drilldown.
  The default value of this arguments are ``-1``.
  If these arguments are set ``-1``, Groonga uses all records for drilldown.

  This argument is useful when filtered result may be very large.
  Because a drilldown against large filtered result may be slow.
  We can limit the max number of records to be used for drilldown by this feature.

  Here is an example to limit the max number of records to be used for drilldown.
  The last 2 records, ``{\"_id\": 4, \"tag\": \"Senna\"}`` and ``{\"_id\": 5, \"tag\": \"Senna\"}``, aren't used.

  .. code-block::

      table_create Entries TABLE_HASH_KEY ShortText
      column_create Entries content COLUMN_SCALAR Text
      column_create Entries n_likes COLUMN_SCALAR UInt32
      column_create Entries tag COLUMN_SCALAR ShortText

      table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
      column_create Terms entries_key_index COLUMN_INDEX|WITH_POSITION Entries _key
      column_create Terms entries_content_index COLUMN_INDEX|WITH_POSITION Entries content
      load --table Entries
      [
      {"_key":    "The first post!",
       "content": "Welcome! This is my first post!",
       "n_likes": 5,
       "tag": "Hello"},
      {"_key":    "Groonga",
       "content": "I started to use Groonga. It's very fast!",
       "n_likes": 10,
       "tag": "Groonga"},
      {"_key":    "Mroonga",
       "content": "I also started to use Mroonga. It's also very fast! Really fast!",
       "n_likes": 15,
       "tag": "Groonga"},
      {"_key":    "Good-bye Senna",
       "content": "I migrated all Senna system!",
       "n_likes": 3,
       "tag": "Senna"},
      {"_key":    "Good-bye Tritonn",
       "content": "I also migrated all Tritonn system!",
       "n_likes": 3,
       "tag": "Senna"}
      ]

      select Entries \
        --limit -1 \
        --output_columns _id,tag \
        --drilldown tag \
        --drilldown_max_n_target_records 3
      [
        [
          0, 
          1337566253.89858, 
          0.000355720520019531
        ], 
        [
          [
            [
              5
            ], 
            [
              [
                "_id", 
                "UInt32"
              ], 
              [
                "tag", 
                "ShortText"
              ]
            ], 
            [
              1, 
              "Hello"
            ], 
            [
              2, 
              "Groonga"
            ], 
            [
              3, 
              "Groonga"
            ], 
            [
              4, 
              "Senna"
            ], 
            [
              5, 
              "Senna"
            ]
          ], 
          [
            [
              2
            ], 
            [
              [
                "_key", 
                "ShortText"
              ], 
              [
                "_nsubrecs", 
                "Int32"
              ]
            ], 
            [
              "Hello", 
              1
            ], 
            [
              "Groonga", 
              2
            ]
          ]
        ]
      ]

* [httpd] Updated bundled nginx to 1.21.6.

.. _release-11-1-3:

Release 11.1.3 - 2022-01-29
---------------------------

Improvements
------------

* [:doc:`reference/functions/snippet`] Added support for using the keyword of 32 or more. [GitHub#1313][Pathched by Takashi Hashida]

  We could not specify the keyword of 32 or more with ``snippet`` until now.
  However, we can specify the keyword of 32 or more by this improvement as below.

  .. code-block::

      table_create Entries TABLE_NO_KEY
      column_create Entries content COLUMN_SCALAR ShortText

      load --table Entries
      [
      {"content": "Groonga is a fast and accurate full text search engine based on inverted index. One of the characteristics of Groonga is that a newly registered document instantly appears in search results. Also, Groonga allows updates without read locks. These characteristics result in superior performance on real-time applications.\nGroonga is also a column-oriented database management system (DBMS). Compared with well-known row-oriented systems, such as MySQL and PostgreSQL, column-oriented systems are more suited for aggregate queries. Due to this advantage, Groonga can cover weakness of row-oriented systems.\nThe basic functions of Groonga are provided in a C library. Also, libraries for using Groonga in other languages, such as Ruby, are provided by related projects. In addition, groonga-based storage engines are provided for MySQL and PostgreSQL. These libraries and storage engines allow any application to use Groonga. See usage examples."},
      {"content": "In widely used DBMSs, updates are immediately processed, for example, a newly registered record appears in the result of the next query. In contrast, some full text search engines do not support instant updates, because it is difficult to dynamically update inverted indexes, the underlying data structure.\nGroonga also uses inverted indexes but supports instant updates. In addition, Groonga allows you to search documents even when updating the document collection. Due to these superior characteristics, Groonga is very flexible as a full text search engine. Also, Groonga always shows good performance because it divides a large task, inverted index merging, into smaller tasks."}
      ]

      select Entries \
        --output_columns ' \
        snippet(content, \
        "groonga", "inverted", "index", "fast", "full", "text", "search", "engine", "registered", "document", \
        "results", "appears", "also", "system", "libraries", "for", "mysql", "postgresql", "column-oriented", "dbms", \
        "basic", "ruby", "projects", "storage", "allow", "application", "usage", "sql", "well-known", "real-time", \
        "weakness", "merging", "performance", "superior", "large", "dynamically", "difficult", "query", "examples", "divides", \
        { \
          "default_open_tag": "[", \
          "default_close_tag": "]", \
          "width" : 2048 \
        })'
      [
        [
          0,
          1643165838.691991,
          0.0003311634063720703
        ],
        [
          [
            [
              2
            ],
            [
              [
                "snippet",
                null
              ]
            ],
            [
              [
                "[Groonga] is a [fast] and accurate [full] [text] [search] [engine] based on [inverted] [index]. One of the characteristics of [Groonga] is that a newly [registered] [document] instantly [appears] in [search] [results]. [Also], [Groonga] [allow]s updates without read locks. These characteristics result in [superior] [performance] on [real-time] [application]s.\n[Groonga] is [also] a [column-oriented] database management [system] ([DBMS]). Compared with [well-known] row-oriented [system]s, such as [MySQL] and [PostgreSQL], [column-oriented] [system]s are more suited [for] aggregate queries. Due to this advantage, [Groonga] can cover [weakness] of row-oriented [system]s.\nThe [basic] functions of [Groonga] are provided in a C library. [Also], [libraries] [for] using [Groonga] in other languages, such as [Ruby], are provided by related [projects]. In addition, [groonga]-based [storage] [engine]s are provided [for] [MySQL] and [PostgreSQL]. These [libraries] and [storage] [engine]s [allow] any [application] to use [Groonga]. See [usage] [examples]."
              ]
            ],
            [
              [
                "In widely used [DBMS]s, updates are immediately processed, [for] example, a newly [registered] record [appears] in the result of the next [query]. In contrast, some [full] [text] [search] [engine]s do not support instant updates, because it is [difficult] to [dynamically] update [inverted] [index]es, the underlying data structure.\n[Groonga] [also] uses [inverted] [index]es but supports instant updates. In addition, [Groonga] [allow]s you to [search] [document]s even when updating the [document] collection. Due to these [superior] characteristics, [Groonga] is very flexible as a [full] [text] [search] [engine]. [Also], [Groonga] always shows good [performance] because it [divides] a [large] task, [inverted] [index] [merging], into smaller tasks."
              ]
            ]
          ]
        ]
      ]

* [:doc:`reference/normalizers/normalizer_nfkc130`] Added a new option ``remove_symbol``.

  This option removes symbols (e.g. #, !, ", &, %, ...) from the string that the target of normalizing as below.

  .. code-block::

       normalize   'NormalizerNFKC130("remove_symbol", true)'   "#This & is %% a pen."   WITH_TYPES
       [
         [
           0,
           1643595008.729597,
           0.0005540847778320312
         ],
         {
           "normalized": "this  is  a pen",
           "types": [
             "alpha",
             "alpha",
             "alpha",
             "alpha",
             "others",
             "others",
             "alpha",
             "alpha",
             "others",
             "others",
             "alpha",
             "others",
             "alpha",
             "alpha",
             "alpha"
           ],
           "checks": [
           ]
         }
       ]

* [:doc:`/install/almalinux`] Added support for AlmaLinux 8 on ARM64.

* [httpd] Updated bundled nginx to 1.21.5.

* [Documentation] Fixed a typo in :doc:`reference/commands/ruby_eval`. [GitHub#1317][Pathched by wi24rd]

* [:doc:`/install/ubuntu`] Dropped Ubuntu 21.04 (Hirsute Hippo) support.

  * Because Ubuntu 21.04 reached EOL January 20, 2022.

Fixes
-----

* [:doc:`/reference/commands/load`] Fixed a crash bug when we load data with specifying a nonexistent column.

  This bug only occurs when we specify ``apache-arrow`` into ``input_type`` as the argument of ``load``.

* Fixed a bug that the version up of Groonga failed Because the version up of arrow-libs on which Groonga depends. [groonga-talk,540][Reported by Josep Sanz][Gitter,61eaaa306d9ba23328d23ce1][Reported by shibanao4870][GitHub#1316][Reported by Keitaro YOSHIMURA]

  However, if arrow-libs update a major version, this problem reproduces.
  In this case, we will handle that by rebuilding the Groonga package.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* Takashi Hashida
* wi24rd
* Josep Sanz
* Keitaro YOSHIMURA
* shibanao4870

.. _release-11-1-1:

Release 11.1.1 - 2021-12-29
---------------------------

Improvements
------------

* [:doc:`reference/commands/select`] Added support for near phrase product search.

  This feature is a shortcut of ``'*NP"..." OR *NP"..." OR ...'``.
  For example, we can use ``*NPP`` instead of the expression that execute mulitiple
  ``*NP`` with ``query`` as below.

  .. code-block::

     query ("title * 10 || content",
            "*NP"a 1 x" OR
             *NP"a 1 y" OR
             *NP"a 1 z" OR
             *NP"a 2 x" OR
             *NP"a 2 y" OR
             *NP"a 2 z" OR
             *NP"a 3 x" OR
             *NP"a 3 y" OR
             *NP"a 3 z" OR
             *NP"b 1 x" OR
             *NP"b 1 y" OR
             *NP"b 1 z" OR
             *NP"b 2 x" OR
             *NP"b 2 y" OR
             *NP"b 2 z" OR
             *NP"b 3 x" OR
             *NP"b 3 y" OR
             *NP"b 3 z"")

  We can be written as ``*NPP"(a b) (1 2 3) (x y z)"`` the above expression by this feature.
  In addition, ``*NPP"(a b) (1 2 3) (x y z)"`` is faster than ``'*NP"..." OR *NP"..." OR ...'``.

  .. code-block::

     query ("title * 10 || content",
            "*NPP"(a b) (1 2 3) (x y z)"")

  We implements this feature for improving performance near phrase search
  like ``'*NP"..." OR *NP"..." OR ...'``.

* [:doc:`reference/commands/select`] Added support for order near phrase product search.

  This feature is a shortcut of ``'*ONP"..." OR *ONP"..." OR ...'``.
  For example, we can use ``*ONPP`` instead of the expression that execute multiple
  ``*ONP`` with ``query`` as below.

  .. code-block::

     query ("title * 10 || content",
            "*ONP"a 1 x" OR
             *ONP"a 1 y" OR
             *ONP"a 1 z" OR
             *ONP"a 2 x" OR
             *ONP"a 2 y" OR
             *ONP"a 2 z" OR
             *ONP"a 3 x" OR
             *ONP"a 3 y" OR
             *ONP"a 3 z" OR
             *ONP"b 1 x" OR
             *ONP"b 1 y" OR
             *ONP"b 1 z" OR
             *ONP"b 2 x" OR
             *ONP"b 2 y" OR
             *ONP"b 2 z" OR
             *ONP"b 3 x" OR
             *ONP"b 3 y" OR
             *ONP"b 3 z"")

  We can be written as ``*ONPP"(a b) (1 2 3) (x y z)"`` the above expression by this feature.
  In addition, ``*ONPP"(a b) (1 2 3) (x y z)"`` is faster than ``'*ONP"..." OR *ONP"..." OR ...'``.

  .. code-block::

     query ("title * 10 || content",
            "*ONPP"(a b) (1 2 3) (x y z)"")

  We implements this feature for improving performance near phrase search
  like ``'*ONP"..." OR *ONP"..." OR ...'``.

* [:doc:`/reference/commands/request_cancel`] Groonga became easily detects ``request_cancel`` while executing a search.

  Because we added more checks of return code to detect ``request_cancel``.

* [:doc:`/reference/commands/thread_dump`] Added a new command ``thread_dump``

  Currently, this command works only on Windows.

  We can put a backtrace of all threads into a log as logs of NOTICE level
  at the time of running this command.

  This feature is useful when we solve a problem such as Groonga doesn't return a response.

* [:doc:`/install/centos`] Dropped support for CentOS 8.

  Because CentOS 8 will reach EOL at 2021-12-31.

Fixes
-----

* Fixed a bug that we can't remove a index column with invalid parameter. [GitHub#1301][Patched by Takashi Hashida]

  * For example, we can't remove a table when we create an invalid index column with ``column_create`` as below.

    .. code-block::

       table_create Statuses TABLE_NO_KEY
       column_create Statuses start_time COLUMN_SCALAR UInt16
       column_create Statuses end_time COLUMN_SCALAR UInt16

       table_create Times TABLE_PAT_KEY UInt16
       column_create Times statuses COLUMN_INDEX Statuses start_time,end_time
       [
         [
           -22,
           1639037503.16114,
           0.003981828689575195,
           "grn_obj_set_info(): GRN_INFO_SOURCE: multi column index must be created with WITH_SECTION flag: <Times.statuses>",
           [
             [
               "grn_obj_set_info_source_validate",
               "../../groonga/lib/db.c",
               9605
             ],
             [
               "/tmp/d.grn",
               6,
               "column_create Times statuses COLUMN_INDEX Statuses start_time,end_time"
             ]
           ]
         ],
         false
       ]
       table_remove Times
       [
         [
           -22,
           1639037503.16515,
           0.0005414485931396484,
           "[object][remove] column is broken: <Times.statuses>",
           [
             [
               "remove_columns",
               "../../groonga/lib/db.c",
               10649
             ],
             [
               "/tmp/d.grn",
               8,
               "table_remove Times"
             ]
           ]
         ],
         false
       ]

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* Takashi Hashida

.. _release-11-1-0:

Release 11.1.0 - 2021-11-29
---------------------------

Improvements
------------

* [:doc:`/reference/commands/load`] Added support for ISO 8601 time format.[GitHub#1228][Patched by Takashi Hashida]

  ``load`` support the following format by this modification.

    * YYYY-MM-ddThh:mm:ss.sZ
    * YYYY-MM-ddThh:mm:ss.s+10:00
    * YYYY-MM-ddThh:mm:ss.s-10:00

  We can also use ``t`` and ``z`` characters instead of ``T`` and ``Z`` in this syntax.
  We can also use ``/`` character instead of ``-`` in this syntax. However, note that this is not an ISO 8601 format.
  This format is present for compatibility.

  .. code-block::

     plugin_register functions/time

     table_create Logs TABLE_NO_KEY
     column_create Logs case COLUMN_SCALAR ShortText
     column_create Logs created_at COLUMN_SCALAR Time
     column_create Logs created_at_text COLUMN_SCALAR ShortText

     load --table Logs
     [
     {"case": "timezone: Z", "created_at": "2000-01-01T10:00:00Z", "created_at_text": "2000-01-01T10:00:00Z"},
     {"case": "timezone: z", "created_at": "2000-01-01t10:00:00z", "created_at_text": "2000-01-01T10:00:00z"},
     {"case": "timezone: 00:00", "created_at": "2000-01-01T10:00:00+00:00", "created_at_text": "2000-01-01T10:00:00+00:00"},
     {"case": "timezone: +01:01", "created_at": "2000-01-01T11:01:00+01:01", "created_at_text": "2000-01-01T11:01:00+01:01"},
     {"case": "timezone: +11:11", "created_at": "2000-01-01T21:11:00+11:11", "created_at_text": "2000-01-01T21:11:00+11:11"},
     {"case": "timezone: -01:01", "created_at": "2000-01-01T08:59:00-01:01", "created_at_text": "2000-01-01T08:59:00-01:01"},
     {"case": "timezone: -11:11", "created_at": "1999-12-31T22:49:00-11:11", "created_at_text": "1999-12-31T22:49:00-11:11"},
     {"case": "timezone hour threshold: +23:00", "created_at": "2000-01-02T09:00:00+23:00", "created_at_text": "2000-01-02T09:00:00+23:00"},
     {"case": "timezone minute threshold: +00:59", "created_at": "2000-01-01T10:59:00+00:59", "created_at_text": "2000-01-01T10:59:00+00:59"},
     {"case": "timezone omitting minute: +01", "created_at": "2000-01-01T11:00:00+01", "created_at_text": "2000-01-01T11:00:00+01"},
     {"case": "timezone omitting minute: -01", "created_at": "2000-01-01T09:00:00-01", "created_at_text": "2000-01-01T09:00:00-01"},
     {"case": "timezone: localtime", "created_at": "2000-01-01T19:00:00", "created_at_text": "2000-01-01T19:00:00"},
     {"case": "compatible: date delimiter: /", "created_at": "2000/01/01T10:00:00Z", "created_at_text": "2000/01/01T10:00:00Z"},
     {"case": "decimal", "created_at": "2000-01-01T11:01:00.123+01:01", "created_at_text": "2000-01-01T11:01:00.123+01:01"}
     ]

     select Logs \
       --limit -1 \
       --output_columns "case, time_format_iso8601(created_at), created_at_text"
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             14
           ],
           [
             [
               "case",
               "ShortText"
             ],
             [
               "time_format_iso8601",
               null
             ],
             [
               "created_at_text",
               "ShortText"
             ]
           ],
           [
             "timezone: Z",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T10:00:00Z"
           ],
           [
             "timezone: z",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T10:00:00z"
           ],
           [
             "timezone: 00:00",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T10:00:00+00:00"
           ],
           [
             "timezone: +01:01",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T11:01:00+01:01"
           ],
           [
             "timezone: +11:11",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T21:11:00+11:11"
           ],
           [
             "timezone: -01:01",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T08:59:00-01:01"
           ],
           [
             "timezone: -11:11",
             "2000-01-01T19:00:00.000000+09:00",
             "1999-12-31T22:49:00-11:11"
           ],
           [
             "timezone hour threshold: +23:00",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-02T09:00:00+23:00"
           ],
           [
             "timezone minute threshold: +00:59",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T10:59:00+00:59"
           ],
           [
             "timezone omitting minute: +01",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T11:00:00+01"
           ],
           [
             "timezone omitting minute: -01",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T09:00:00-01"
           ],
           [
             "timezone: localtime",
             "2000-01-01T19:00:00.000000+09:00",
             "2000-01-01T19:00:00"
           ],
           [
             "compatible: date delimiter: /",
             "2000-01-01T19:00:00.000000+09:00",
             "2000/01/01T10:00:00Z"
           ],
           [
             "decimal",
             "2000-01-01T19:00:00.123000+09:00",
             "2000-01-01T11:01:00.123+01:01"
           ]
         ]
       ]
     ]

* [:doc:`reference/commands/select`] Added a new ``query_flags`` ``DISABLE_PREFIX_SEARCH``.

  We can use the prefix search operators ``^`` and ``*`` as search keywords
  by ``DISABLE_PREFIX_SEARCH`` as below.

  This feature is useful if we want to search documents including ``^`` and ``*``.

  .. code-block::

     table_create Users TABLE_PAT_KEY ShortText

     load --table Users
     [
     {"_key": "alice"},
     {"_key": "alan"},
     {"_key": "ba*"}
     ]

     select Users \
       --match_columns "_key" \
       --query "a*" \
       --query_flags "DISABLE_PREFIX_SEARCH"
     [[0,0.0,0.0],[[[1],[["_id","UInt32"],["_key","ShortText"]],[3,"ba*"]]]]


  .. code-block::

     table_create Users TABLE_PAT_KEY ShortText

     load --table Users
     [
     {"_key": "alice"},
     {"_key": "alan"},
     {"_key": "^a"}
     ]

     select Users \
       --query "_key:^a" \
       --query_flags "ALLOW_COLUMN|DISABLE_PREFIX_SEARCH"
     [[0,0.0,0.0],[[[1],[["_id","UInt32"],["_key","ShortText"]],[3,"^a"]]]]

* [:doc:`reference/commands/select`] Added a new ``query_flags`` ``DISABLE_AND_NOT``.

  We can use ``AND NOT`` operators ``-`` as search keywords
  by ``DISABLE_AND_NOT`` as below.

  This feature is useful if we want to search documents including ``-``.

  .. code-block::

    table_create Users TABLE_PAT_KEY ShortText

    load --table Users
    [
    {"_key": "alice"},
    {"_key": "bob"},
    {"_key": "cab-"}
    ]

    select Users   --match_columns "_key"   --query "b - a"   --query_flags "DISABLE_AND_NOT"
    [[0,0.0,0.0],[[[1],[["_id","UInt32"],["_key","ShortText"]],[3,"cab-"]]]]

Fixes
-----

* [The browser based administration tool] Fixed a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list. [GitHub#1186][Patched by Takashi Hashida]

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* Takashi Hashida

.. _release-11-0-9:

Release 11.0.9 - 2021-11-04
---------------------------

Improvements
------------

* [:doc:`reference/functions/snippet`] Added a new option ``delimiter_regexp`` for detecting snippet delimiter with regular expression.

  :doc:`reference/functions/snippet` extracts text around search keywords.
  We call the text that is extracted by :doc:`reference/functions/snippet` snippet.

  Normally, :doc:`reference/functions/snippet` () returns the text of 200 bytes around search keywords.
  However, :doc:`reference/functions/snippet` () gives no thought to a delimiter of sentences.
  The snippet may be composed of multi sentences.

  ``delimiter_regexp`` option is useful if we want to only extract the text of the same sentence as search keywords.
  For example, we can use ``\.\s*`` to extract only text in the target sentence as below.
  Note that you need to escape ``\`` in string.

    .. code-block::

       table_create Documents TABLE_NO_KEY
       column_create Documents content COLUMN_SCALAR Text

       table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram  --normalizer NormalizerAuto
       column_create Terms documents_content_index COLUMN_INDEX|WITH_POSITION Documents content

       load --table Documents
       [
       ["content"],
       ["Groonga is a fast and accurate full text search engine based on inverted index. One of the characteristics of groonga is that a newly registered document instantly appears in search results. Also, groonga allows updates without read locks. These characteristics result in superior performance on real-time applications."],
       ["Groonga is also a column-oriented database management system (DBMS). Compared with well-known row-oriented systems, such as MySQL and PostgreSQL, column-oriented systems are more suited for aggregate queries. Due to this advantage, groonga can cover weakness of row-oriented systems."]
       ]

       select Documents \
         --output_columns 'snippet(content, \
                                   { \
                                      "default_open_tag": "[", \
                                      "default_close_tag": "]", \
                                      "delimiter_regexp": "\\\\.\\\\s*" \
                                   })' \
         --match_columns content \
         --query "fast performance"
       [
         [
           0,
           1337566253.89858,
           0.000355720520019531
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "snippet",
                 null
               ]
             ],
             [
               [
                 "Groonga is a [fast] and accurate full text search engine based on inverted index",
                 "These characteristics result in superior [performance] on real-time applications"
               ]
             ]
           ]
         ]
       ]

* [:doc:`reference/window_functions/window_rank`] Added a new function ``window_rank()``.

  * We can calculate a rank that includes a gap of each record.
    Normally, the rank isn’t incremented when multiple records that are the same order.
    For example, if values of sort keys are 100, 100, 200 then the ranks of them
    are 1, 1, 3.
    The rank of the last record is 3 not 2 because there are two 1 rank records.

    This is similar to :doc:`reference/window_functions/window_record_number`.
    However, :doc:`reference/window_functions/window_record_number` gives no thought to gap.

    .. code-block::

       table_create Points TABLE_NO_KEY
       column_create Points game COLUMN_SCALAR ShortText
       column_create Points score COLUMN_SCALAR UInt32

       load --table Points
       [
       ["game",  "score"],
       ["game1", 100],
       ["game1", 200],
       ["game1", 100],
       ["game1", 400],
       ["game2", 150],
       ["game2", 200],
       ["game2", 200],
       ["game2", 200]
       ]

       select Points \
         --columns[rank].stage filtered \
         --columns[rank].value 'window_rank()' \
         --columns[rank].type UInt32 \
         --columns[rank].window.sort_keys score \
         --output_columns 'game, score, rank' \
         --sort_keys score
       [
         [
           0,
           1337566253.89858,
           0.000355720520019531
         ],
         [
           [
             [
               8
             ],
             [
               [
                 "game",
                 "ShortText"
               ],
               [
                 "score",
                 "UInt32"
               ],
               [
                 "rank",
                 "UInt32"
               ]
             ],
             [
               "game1",
               100,
               1
             ],
             [
               "game1",
               100,
               1
             ],
             [
               "game2",
               150,
               3
             ],
             [
               "game2",
               200,
               4
             ],
             [
               "game2",
               200,
               4
             ],
             [
               "game1",
               200,
               4
             ],
             [
               "game2",
               200,
               4
             ],
             [
               "game1",
               400,
               8
             ]
           ]
         ]
       ]

* [:doc:`/reference/functions/in_values`] Added support for auto cast when we search tables.

  For example, if we load values of ``UInt32`` into a table that a key type is ``UInt64``, Groonga cast the values to ``UInt64`` automatically when we search the table with ``in_values()``. However, ``in_values(_key, 10)`` doesn't work with ``UInt64`` key table. Because 10 is parsed as ``Int32``.

  .. code-block::

     table_create Numbers TABLE_HASH_KEY UInt64
     load --table Numbers
     [
     {"_key": 100},
     {"_key": 200},
     {"_key": 300}
     ]

     select Numbers   --output_columns _key   --filter 'in_values(_key, 200, 100)'   --sortby _id
     [[0,0.0,0.0],[[[2],[["_key","UInt64"]],[100],[200]]]]

* [httpd] Updated bundled nginx to 1.21.3.

* [:doc:`/install/almalinux`] Added support for AlmaLinux 8.

* [:doc:`/install/ubuntu`] Added support for Ubuntu 21.10 (Impish Indri).

Fixes
-----

* Fixed a bug that Groonga doesn't return a response when an error occurred
  in command (e.g. sytax error in filter).

  * This bug only occurs when we use ``--output_type apache-arrow``.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

.. _release-11-0-7:

Release 11.0.7 - 2021-09-29
---------------------------

Improvements
------------

* [:doc:`/reference/commands/load`] Added support for casting a string like as "[int, int,...]" to a vector of integer like as [int, int,...].

  For example, Groonga handle as a vector of integer like as [1, -2] even if we load vector of string like as "[1, -2]" as below.

    .. code-block::

       table_create Data TABLE_NO_KEY
       column_create Data numbers COLUMN_VECTOR Int16
       table_create Numbers TABLE_PAT_KEY Int16
       column_create Numbers data_numbers COLUMN_INDEX Data numbers

       load --table Data
       [
       {"numbers": "[1, -2]"},
       {"numbers": "[-3, 4]"}
       ]

       dump   --dump_plugins no   --dump_schema no
       load --table Data
       [
       ["_id","numbers"],
       [1,[1,-2]],
       [2,[-3,4]]
       ]

       column_create Numbers data_numbers COLUMN_INDEX Data numbers
       select Data --filter 'numbers @ -2'
       [[0,0.0,0.0],[[[1],[["_id","UInt32"],["numbers","Int16"]],[1,[1,-2]]]]]

  This feature supports for the floowings types.

    * Int8
    * UInt8
    * Int16
    * UInt16
    * Int32
    * UInt32
    * Int64
    * UInt64

* [:doc:`/reference/commands/load`] Added support for loading a JSON array expressed as a text string as a vector of string.

  For example, Groonga handle as a vector that has two elements like as ["hello", "world"] if we load JSON array expressed as a text string like as "[\"hello\", \"world\"]" as below.

  .. code-block::

     table_create Data TABLE_NO_KEY
     [[0,0.0,0.0],true]
     column_create Data strings COLUMN_VECTOR ShortText
     [[0,0.0,0.0],true]
     table_create Terms TABLE_PAT_KEY ShortText   --normalizer NormalizerNFKC130   --default_tokenizer TokenNgram
     [[0,0.0,0.0],true]
     column_create Terms data_strings COLUMN_INDEX Data strings
     [[0,0.0,0.0],true]
     load --table Data
     [
     {"strings": "[\"Hello\", \"World\"]"},
     {"strings": "[\"Good-bye\", \"World\"]"}
     ]
     [[0,0.0,0.0],2]
     dump   --dump_plugins no   --dump_schema no
     load --table Data
     [
     ["_id","strings"],
     [1,["Hello","World"]],
     [2,["Good-bye","World"]]
     ]

     column_create Terms data_strings COLUMN_INDEX Data strings
     select Data --filter 'strings @ "bye"'
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "strings",
               "ShortText"
             ]
           ],
           [
             2,
             [
               "Good-bye",
               "World"
             ]
           ]
         ]
       ]
     ]

  In before version, Groonga handled as a vector that had one element like as ["[\"hello\", \"world\"]"] if we loaded JSON array expressed as a text string like as "[\"hello\", \"world\"]".

* [Documentation] Added a documentation about the following items.

  * [:doc:`reference/commands/column_create`] Added a documentation about ``WEIGHT_FLOAT32`` flag.

  * [:doc:`reference/normalizers/normalizer_nfkc121`] Added a documentation about ``NormalizerNFKC121``.

  * [:doc:`reference/normalizers/normalizer_nfkc130`] Added a documentation about ``NormalizerNFKC130``.

  * [:doc:`reference/normalizers/normalizer_table`] Added a documentation about ``NormalizerTable``.

* Updated to 3.0.0 that the version of Apache Arrow that Groonga requires. [GitHub#1265][Patched by Takashi Hashida]

Fixes
-----

* Fixed a memory leak when we created a table with a tokenizer with invalid option.

* Fixed a bug that may not add a new entry in Hash table.

  This bug only occurs in Groonga 11.0.6, and it may occur if we quite a lot of add and delete data.
  If this bug occurs in your environment, you can resolve this problem by executing the following steps.

  1. We upgrade Groonga to 11.0.7 or later from 11.0.6.
  2. We make a new table that has the same schema as the original table.
  3. We copy data to the new table from the original table.

* [Windows] Fixed a resource leak when Groonga fail open a new file caused by out of memory.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* Takashi Hashida

.. _release-11-0-6:

Release 11.0.6 - 2021-08-29
---------------------------

  .. warning::

     Groonga 11.0.6 has had a bug that may not add a new entry in Hash table.

     We fixed this bug on Groonga 11.0.7. This bug only occurs in Groonga 11.0.6.
     Therefore, if you were using Groonga 11.0.6, we highly recommended that
     you use Groonga 11.0.7 or later.

Improvements
------------

* Added support for recovering on crash. (experimental)

  This is a experimental feature. Currently, this feature is still not stable.

  If Groonga crashes, it recovers the database automatically when it opens a database for the first time since the crash. However, This feature can't recover the database automatically in all crash cases.
  We need to recover the database manually depending on timing even if this feature enables.

  Groonga execute WAL (write ahead log) when this feature is enable.
  We can dump WAL by the following tools, but currently, users doesn't need to use them.

    * [:doc:`reference/executables/grndb`] ``dump-wal`` command.
    * ``dump-wal.rb`` scritp.

* [:doc:`reference/commands/cache_limit`] Groonga remove cache when we execute ``cache_limit 0``. [GitHub#1224][Reported by higchi]

  Groonga stores query cache to internally table.
  The maximum total size of keys of this table is 4GiB. Because this table is hash table.
  Therefore, If we execute many huge queries, Groonga may be unable to store query cache, because the maximum total size of keys may be over 4GiB.
  In such cases, We can clear the table for query cache by using ``cache_limit 0``, and Groonga can store query cache 

Fixes
-----

* Fixed a bug that Groonga doesn't clear lock when some threads open the same object around the same time.

  If some threads open the same object around the same time, threads except for a thread that executes the opening object at first are waiting for opening the target object.
  At this time, threads that wait for an opening object take locks, but these locks are not released.
  Therefore, these locks remain until Groonga's process is restarted in the above case, and a new thread can't also open the object all the time until Groonga's process is restarted.

  However, this bug rarely happens. Because a time of a thread open the object is a very short time. 

* [:doc:`reference/functions/query_parallel_or`] Fixed a bug that result may be different from the ``query()``.

  For example, If we used ``query("tags || tags2", "beginner man")``, the following record was a match, but if we used ``query_parallel_or("tags || tags2", "beginner man")``, the following record wasn't a match until now.

  * ``{"_key": "Bob",   "comment": "Hey!",       "tags": ["expert", "man"], "tags2": ["beginner"]}``

  Even if we use ``query_parallel_or("tags || tags2", "beginner man")``, the above record is match by this modification.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* Groonga may not return records that should match caused by ``GRN_II_CURSOR_SET_MIN_ENABLE``.

Thanks
------

* higchi

.. _release-11-0-5:

Release 11.0.5 - 2021-07-29
---------------------------

Improvements
------------

* [:doc:`/reference/normalizers`] Added support for multiple normalizers.

  We can specify multiple normalizers by ``--notmalizers`` option when we create a table since this release.
  If we can also specify them by  ``--normalizer`` existing option because of compatibility.

  We added ``NormalizerTable`` for customizing a normalizer in Groonga 11.0.4.
  We can more flexibly behavior of the normalizer by combining ``NormalizerTable`` with existing normalizer.

  For example, this feature is useful in the following case.

    * Search for a telephone number. However, we import data handwritten by OCR.
      If data is handwritten, OCR may misunderstand a number and string(e.g. 5 and S).

  The details are as follows.

  .. code-block::

     table_create Normalizations TABLE_PAT_KEY ShortText
     column_create Normalizations normalized COLUMN_SCALAR ShortText
     load --table Normalizations
     [
     {"_key": "s", "normalized": "5"}
     ]


     table_create Tels TABLE_NO_KEY
     column_create Tels tel COLUMN_SCALAR ShortText

     table_create TelsIndex TABLE_PAT_KEY ShortText \
       --normalizers 'NormalizerNFKC130("unify_hyphen_and_prolonged_sound_mark", true), \
                      NormalizerTable("column", "Normalizations.normalized")' \
       --default_tokenizer 'TokenNgram("loose_symbol", true, "loose_blank", true)'
     column_create TelsIndex tel_index COLUMN_INDEX|WITH_SECTION Tels tel

     load --table Tels
     [
     {"tel": "03-4S-1234"}
     {"tel": "03-45-9876"}
     ]

     select --table Tels \
       --filter 'tel @ "03-45-1234"'
     [
       [
         0,
         1625227424.560146,
         0.0001730918884277344
       ],
       [
         [
           [
             1
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "tel",
               "ShortText"
             ]
           ],
           [
             1,
             "03-4S-1234"
           ]
         ]
       ]
     ]

  Existing normalizers can't meet in such case, but we can meet it by combining ``NormalizerTable`` with existing normalizer since this release.

* [:doc:`reference/functions/query_parallel_or`][:doc:`reference/functions/query`] Added support for customizing thresholds for sequential search.

  We can customize thresholds in each queries whether to use sequential search by the following options.

    * ``{"max_n_enough_filtered_records": xx}``

      ``max_n_enough_filtered_records`` specify the number of records.
      ``query`` or ``query_parallel_or`` use sequential search when they seems to narrow down until under this number.

    * ``{"enough_filtered_ratio": x.x}``

      ``enough_filtered_ratio`` specify percentage of total.
      ``query`` or ``query_parallel_or`` use sequential search when they seems to narrow down until under this percentage.
      For example, if we specify ``{"enough_filtered_ratio": 0.5}``, ``query`` or ``query_parallel_or`` use sequential search when they seems to narrow down until half of the whole.

  The details are as follows.

    .. code-block::

       table_create Products TABLE_NO_KEY
       column_create Products name COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
       column_create Terms products_name COLUMN_INDEX Products name

       load --table Products
       [
       ["name"],
       ["Groonga"],
       ["Mroonga"],
       ["Rroonga"],
       ["PGroonga"],
       ["Ruby"],
       ["PostgreSQL"]
       ]

       select \
         --table Products \
         --filter 'query("name", "r name:Ruby", {"enough_filtered_ratio": 0.5})'

    .. code-block::

       table_create Products TABLE_NO_KEY
       column_create Products name COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
       column_create Terms products_name COLUMN_INDEX Products name

       load --table Products
       [
       ["name"],
       ["Groonga"],
       ["Mroonga"],
       ["Rroonga"],
       ["PGroonga"],
       ["Ruby"],
       ["PostgreSQL"]
       ]

       select \
         --table Products \
         --filter 'query("name", "r name:Ruby", {"max_n_enough_filtered_records": 10})'

* [:doc:`/reference/functions/between`][:doc:`/reference/functions/in_values`] Added support for customizing thresholds for sequential search.

  [:doc:`/reference/functions/between`] and [:doc:`/reference/functions/in_values`] have a feature that they switch to sequential search when the target of search records is narrowed down enough.

  The value of ``GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO`` / ``GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO`` is used as threshold whether Groonga execute sequential search or search with indexes in such a case.

  This behavior is customized by only the following environment variable until now.

  ``in_values()``::

    # Don't use auto sequential search
    GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO=-1
    # Set threshold to 0.02
    GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO=0.02

  ``between()``::

    # Don't use auto sequential search
    GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO=-1
    # Set threshold to 0.02
    GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO=0.02

  if customize by the environment variable, this threshold applies to all queries, but we can specify it in each query by using this feature.

  The details are as follows. We can specify the threshold by using ``{"too_many_index_match_ratio": x.xx}`` option.
  The value type of this option is ``double``.

  .. code-block::

     table_create Memos TABLE_HASH_KEY ShortText
     column_create Memos timestamp COLUMN_SCALAR Time

     table_create Times TABLE_PAT_KEY Time
     column_create Times memos_timestamp COLUMN_INDEX Memos timestamp

     load --table Memos
     [
     {"_key": "001", "timestamp": "2014-11-10 07:25:23"},
     {"_key": "002", "timestamp": "2014-11-10 07:25:24"},
     {"_key": "003", "timestamp": "2014-11-10 07:25:25"},
     {"_key": "004", "timestamp": "2014-11-10 07:25:26"},
     {"_key": "005", "timestamp": "2014-11-10 07:25:27"},
     {"_key": "006", "timestamp": "2014-11-10 07:25:28"},
     {"_key": "007", "timestamp": "2014-11-10 07:25:29"},
     {"_key": "008", "timestamp": "2014-11-10 07:25:30"},
     {"_key": "009", "timestamp": "2014-11-10 07:25:31"},
     {"_key": "010", "timestamp": "2014-11-10 07:25:32"},
     {"_key": "011", "timestamp": "2014-11-10 07:25:33"},
     {"_key": "012", "timestamp": "2014-11-10 07:25:34"},
     {"_key": "013", "timestamp": "2014-11-10 07:25:35"},
     {"_key": "014", "timestamp": "2014-11-10 07:25:36"},
     {"_key": "015", "timestamp": "2014-11-10 07:25:37"},
     {"_key": "016", "timestamp": "2014-11-10 07:25:38"},
     {"_key": "017", "timestamp": "2014-11-10 07:25:39"},
     {"_key": "018", "timestamp": "2014-11-10 07:25:40"},
     {"_key": "019", "timestamp": "2014-11-10 07:25:41"},
     {"_key": "020", "timestamp": "2014-11-10 07:25:42"},
     {"_key": "021", "timestamp": "2014-11-10 07:25:43"},
     {"_key": "022", "timestamp": "2014-11-10 07:25:44"},
     {"_key": "023", "timestamp": "2014-11-10 07:25:45"},
     {"_key": "024", "timestamp": "2014-11-10 07:25:46"},
     {"_key": "025", "timestamp": "2014-11-10 07:25:47"},
     {"_key": "026", "timestamp": "2014-11-10 07:25:48"},
     {"_key": "027", "timestamp": "2014-11-10 07:25:49"},
     {"_key": "028", "timestamp": "2014-11-10 07:25:50"},
     {"_key": "029", "timestamp": "2014-11-10 07:25:51"},
     {"_key": "030", "timestamp": "2014-11-10 07:25:52"},
     {"_key": "031", "timestamp": "2014-11-10 07:25:53"},
     {"_key": "032", "timestamp": "2014-11-10 07:25:54"},
     {"_key": "033", "timestamp": "2014-11-10 07:25:55"},
     {"_key": "034", "timestamp": "2014-11-10 07:25:56"},
     {"_key": "035", "timestamp": "2014-11-10 07:25:57"},
     {"_key": "036", "timestamp": "2014-11-10 07:25:58"},
     {"_key": "037", "timestamp": "2014-11-10 07:25:59"},
     {"_key": "038", "timestamp": "2014-11-10 07:26:00"},
     {"_key": "039", "timestamp": "2014-11-10 07:26:01"},
     {"_key": "040", "timestamp": "2014-11-10 07:26:02"},
     {"_key": "041", "timestamp": "2014-11-10 07:26:03"},
     {"_key": "042", "timestamp": "2014-11-10 07:26:04"},
     {"_key": "043", "timestamp": "2014-11-10 07:26:05"},
     {"_key": "044", "timestamp": "2014-11-10 07:26:06"},
     {"_key": "045", "timestamp": "2014-11-10 07:26:07"},
     {"_key": "046", "timestamp": "2014-11-10 07:26:08"},
     {"_key": "047", "timestamp": "2014-11-10 07:26:09"},
     {"_key": "048", "timestamp": "2014-11-10 07:26:10"},
     {"_key": "049", "timestamp": "2014-11-10 07:26:11"},
     {"_key": "050", "timestamp": "2014-11-10 07:26:12"}
     ]

     select Memos \
       --filter '_key == "003" && \
                 between(timestamp, \
                         "2014-11-10 07:25:24", \
                         "include", \
                         "2014-11-10 07:27:26", \
                         "exclude", \
                         {"too_many_index_match_ratio": 0.03})'

  .. code-block::

     table_create Tags TABLE_HASH_KEY ShortText

     table_create Memos TABLE_HASH_KEY ShortText
     column_create Memos tag COLUMN_SCALAR Tags

     load --table Memos
     [
     {"_key": "Rroonga is fast!", "tag": "Rroonga"},
     {"_key": "Groonga is fast!", "tag": "Groonga"},
     {"_key": "Mroonga is fast!", "tag": "Mroonga"},
     {"_key": "Groonga sticker!", "tag": "Groonga"},
     {"_key": "Groonga is good!", "tag": "Groonga"}
     ]

     column_create Tags memos_tag COLUMN_INDEX Memos tag

     select \
       Memos \
       --filter '_id >= 3 && \
                 in_values(tag, \
                          "Groonga", \
                          {"too_many_index_match_ratio": 0.7})' \
       --output_columns _id,_score,_key,tag

* [:doc:`/reference/functions/between`] Added support for ``GRN_EXPR_OPTIMIZE=yes``.

  ``between()`` supported for optimizing the order of evaluation of a conditional expression.

* [:doc:`reference/functions/query_parallel_or`][:doc:`reference/functions/query`] Added support for specifying group of match_columns as vector. [GitHub#1238][Patched by naoa]

  We can use vector in ``match_columns`` of ``query`` and ``query_parallel_or`` as below.

  .. code-block::

     table_create Users TABLE_NO_KEY
     column_create Users name COLUMN_SCALAR ShortText
     column_create Users memo COLUMN_SCALAR ShortText
     column_create Users tag COLUMN_SCALAR ShortText

     table_create Terms TABLE_PAT_KEY ShortText \
       --default_tokenizer TokenNgram \
       --normalizer NormalizerNFKC130
     column_create Terms name COLUMN_INDEX|WITH_POSITION Users name
     column_create Terms memo COLUMN_INDEX|WITH_POSITION Users memo
     column_create Terms tag COLUMN_INDEX|WITH_POSITION Users tag

     load --table Users
     [
     {"name": "Alice", "memo": "Groonga user", "tag": "Groonga"},
     {"name": "Bob",   "memo": "Rroonga user", "tag": "Rroonga"}
     ]

     select Users \
       --output_columns _score,name \
       --filter 'query(["name * 100", "memo", "tag * 10"], \
                       "Alice OR Groonga")'

* [:doc:`reference/commands/select`] Added support for section and weight in prefix search. [GitHub#1240][Patched by naoa]

  We can use multi column index and adjusting score in prefix search.

  .. code-block::

     table_create Memos TABLE_NO_KEY
     column_create Memos title COLUMN_SCALAR ShortText
     column_create Memos tags COLUMN_VECTOR ShortText

     table_create Terms TABLE_PAT_KEY ShortText
     column_create Terms index COLUMN_INDEX|WITH_SECTION Memos title,tags

     load --table Memos
     [
     {"title": "Groonga", "tags": ["Groonga"]},
     {"title": "Rroonga", "tags": ["Groonga", "Rroonga", "Ruby"]},
     {"title": "Mroonga", "tags": ["Groonga", "Mroonga", "MySQL"]}
     ]

     select Memos \
       --match_columns "Terms.index.title * 2" \
       --query 'G*' \
       --output_columns title,tags,_score
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "title",
               "ShortText"
             ],
             [
               "tags",
               "ShortText"
             ],
             [
               "_score",
               "Int32"
             ]
           ],
           [
             "Groonga",
             [
               "Groonga"
             ],
             2
           ]
         ]
       ]
     ]

* [:doc:`/reference/executables/grndb`] Added support for closing used object immediately in ``grndb recover``.

  We can reduce memory usage by this.
  This may decrease performance but it will be acceptable.

  Note that ``grndb check`` doesn't close used objects immediately yet.

* [:doc:`reference/functions/query_parallel_or`][:doc:`reference/functions/query`] Added support for specifying ``scorer_tf_idf`` in ``match_columns`` as below.

  .. code-block::

     table_create Tags TABLE_HASH_KEY ShortText

     table_create Users TABLE_HASH_KEY ShortText
     column_create Users tags COLUMN_VECTOR Tags

     load --table Users
     [
     {"_key": "Alice",
      "tags": ["beginner", "active"]},
     {"_key": "Bob",
      "tags": ["expert", "passive"]},
     {"_key": "Chris",
      "tags": ["beginner", "passive"]}
     ]

     column_create Tags users COLUMN_INDEX Users tags

     select Users \
       --output_columns _key,_score \
       --sort_keys _id \
       --command_version 3 \
       --filter 'query_parallel_or("scorer_tf_idf(tags)", \
                                   "beginner active")'
     {
       "header": {
         "return_code": 0,
         "start_time": 0.0,
         "elapsed_time": 0.0
       },
       "body": {
         "n_hits": 1,
         "columns": [
           {
             "name": "_key",
             "type": "ShortText"
           },
           {
             "name": "_score",
             "type": "Float"
           }
         ],
         "records": [
           [
             "Alice",
             2.098612308502197
           ]
         ]
       }
     }

* [:doc:`/reference/commands/query_expand`] Added support for weighted increment, decrement, and negative.

  We can specify weight against expanded words.

  If we want to increment score, we use ``>``.
  If we want to decrement score, we use ``<``.

  We can specify the quantity of scores as a number. We can also use a negative numbers in it.

  .. code-block::

     table_create TermExpansions TABLE_NO_KEY
     column_create TermExpansions term COLUMN_SCALAR ShortText
     column_create TermExpansions expansions COLUMN_VECTOR ShortText

     load --table TermExpansions
     [
     {"term": "Rroonga", "expansions": ["Rroonga", "Ruby Groonga"]}
     ]

     query_expand TermExpansions "Groonga <-0.2Rroonga Mroonga" \
       --term_column term \
       --expanded_term_column expansions
     [[0,0.0,0.0],"Groonga <-0.2((Rroonga) OR (Ruby Groonga)) Mroonga"]

* [httpd] Updated bundled nginx to 1.21.1.

* Updated bundled Apache Arrow to 5.0.0.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 20.10 (Groovy Gorilla) support.

  * Because Ubuntu 20.10 reached EOL July 22, 2021.

Fixes
-----

* [:doc:`reference/functions/query_parallel_or`][:doc:`reference/functions/query`] Fixed a bug that if we specify ``query_options`` and the other options, the other options are ignored.

  For example, ``"default_operator": "OR"`` option had been ignored in the following case.

  .. code-block::

     plugin_register token_filters/stop_word

     table_create Memos TABLE_NO_KEY
     column_create Memos content COLUMN_SCALAR ShortText

     table_create Terms TABLE_PAT_KEY ShortText \
       --default_tokenizer TokenBigram \
       --normalizer NormalizerAuto \
       --token_filters TokenFilterStopWord
     column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
     column_create Terms is_stop_word COLUMN_SCALAR Bool

     load --table Terms
     [
     {"_key": "and", "is_stop_word": true}
     ]

     load --table Memos
     [
     {"content": "Hello"},
     {"content": "Hello and Good-bye"},
     {"content": "and"},
     {"content": "Good-bye"}
     ]

     select Memos \
       --filter 'query_parallel_or( \
                   "content", \
                   "Hello and", \
                   {"default_operator": "OR", \
                    "options": {"TokenFilterStopWord.enable": false}})' \
       --match_escalation_threshold -1 \
       --sort_keys -_score
     [
       [
         0,
         0.0,
         0.0
       ],
       [
         [
           [
             1
           ],
           [
             [
               "_id",
               "UInt32"
             ],
             [
               "content",
               "ShortText"
             ]
           ],
           [
             2,
             "Hello and Good-bye"
           ]
         ]
       ]
     ]

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

* If we repeat that we remove any data and load them again, Groonga may not return records that should match.

Thanks
------

* naoa

.. _release-11-0-4:

Release 11.0.4 - 2021-06-29
---------------------------

Improvements
^^^^^^^^^^^^

* [Normalizer] Added support for customized normalizer.

  We define a table for normalize to use this feature.
  We can normalize with use that table.
  In other words, we can use customized normalizer.

  For example, we define that "S" normalize to "5" in the following example.
  The ``Substitutions`` table is for nromalize.

    .. code-block::

       table_create Substitutions TABLE_PAT_KEY ShortText
       column_create Substitutions substituted COLUMN_SCALAR ShortText
       load --table Substitutions
       [
       {"_key": "S", "substituted": "5"}
       ]

       table_create TelLists TABLE_NO_KEY
       column_create TelLists tel COLUMN_SCALAR ShortText

       table_create Terms TABLE_HASH_KEY ShortText \
         --default_tokenizer TokenNgram \
         --normalizer 'NormalizerTable("column", "Substitutions.substituted", \
                                       "report_source_offset", true)'
       column_create Terms tel_index COLUMN_INDEX|WITH_POSITION TelLists tel

       load --table TelLists
       [
       {"tel": "03-4S-1234"}
       ]

       select TelLists --filter 'tel @ "03-45-1234"'
       [
         [
           0,
           1624686303.538532,
           0.001319169998168945
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "tel",
                 "ShortText"
               ]
             ],
             [
               1,
               "03-4S-1234"
             ]
           ]
         ]
       ]

  For example, we can define to the table easy to false recognize words when we input a handwritten data.
  By this, we can normalize incorrect data to correct data.

  Note that we need to reconstruct the index if we updated the table for normalize.

* Added a new command ``object_warm``.

  This commnad ship Groonga's DB to OS's page cache.

  If we never startup Groonga after OS startup, Groonga's DB doesn't exist on OS's page cache
  When Groonga on the first run.
  Therefore, the first operation to Groonga is slow.

  If we execute this command in advance, the first operation to Groonga is fast.
  In Linux, we can do the same by also executing ``cat *.db > dev/null``.
  However, we could not do the same thing in Windows until now.

  By using this command, we can ship Groonga's DB to OS's page cache in both Linux and Windows.
  Then, we can also do that in units of table, column, and index.
  Therefore, we can ship only table, column, and index that we often use to OS's page cache.

  We can execute this command against various targets as below.

    * If we specify ``object_warm --name index_name``, the index is shipped to OS's page cache.
    * If we specify ``object_warm --name column_name``, the column is shipped to OS's page cache.
    * If we specify ``object_warm --name table_name`` is shipped to OS's page cache.
    * If we specify ``object_warm``, whole Groonga's database is shipped to OS's page cache.

  However, note that if OS has not empty space on memory, this command has no effect.

* [:doc:`reference/commands/select`] Added support for adjusting the score of a specific record in ``--filter``.

  We can adjust the score of a specific record by using a oprtator named ``*~``.
  ``*~`` is logical operator same as ``&&`` and ``||``. Therefore, we can use ``*~`` like as ``&&`` ans ``||``.
  Default weight of ``*~`` is -1.

  Therefore, for example, ``'content @ "Groonga" *~ content @ "Mroonga"'`` mean the following operations.

    1. Extract records that match ``'content @ "Groonga"`` and ``content @ "Mroonga"'``.
    2. Add a score as below.

       a. Calculate the score of ``'content @ "Groonga"``.
       b. Calculate the score of ``'content @ "Mroonga"'``.
       c. b's score multiplied by -1 by ``*~``.
       d. The socre of this record is a + b
          Therefore, if a's socre is 1 and b's score is 1, the score of this record  is 1 + (1 * -1) = 0.

  Then, we can specify score quantity by ``*~${score_quantity}``.

  In particular, the following query adjust the score of match records by the following condition(``'content @ "Groonga" *~2.5 content @ "Mroonga")'`` ).

    .. code-block::

       table_create Memos TABLE_NO_KEY
       column_create Memos content COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerAuto
       column_create Terms index COLUMN_INDEX|WITH_POSITION Memos content

       load --table Memos
       [
       {"content": "Groonga is a full text search engine."},
       {"content": "Rroonga is the Ruby bindings of Groonga."},
       {"content": "Mroonga is a MySQL storage engine based of Groonga."}
       ]

       select Memos \
         --command_version 3 \
         --filter 'content @ "Groonga" *~2.5 content @ "Mroonga"' \
         --output_columns 'content, _score' \
         --sort_keys -_score,_id
       {
         "header": {
           "return_code": 0,
           "start_time": 1624605205.641078,
           "elapsed_time": 0.002965450286865234
         },
         "body": {
           "n_hits": 3,
           "columns": [
             {
               "name": "content",
               "type": "ShortText"
             },
             {
               "name": "_score",
               "type": "Float"
             }
           ],
           "records": [
             [
               "Groonga is a full text search engine.",
               1.0
             ],
             [
               "Rroonga is the Ruby bindings of Groonga.",
               1.0
             ],
             [
               "Mroonga is a MySQL storage engine based of Groonga.",
               -1.5
             ]
           ]
         }
       }

  We can do the same by also useing :ref:`select-adjuster` .
  If we use :ref:`select-adjuster` , we need to make ``--filter`` condition and ``--adjuster`` conditon on our application, but we make only ``--filter`` condition on it by this improvement.

  We can also describe filter condition as below by using ``query()``.

    * ``--filter 'content @ "Groonga" *~2.5 content @ "Mroonga"'``

* [:doc:`reference/commands/select`] Added support for ``&&`` with weight.

  We can use ``&&`` with weight by using ``*<`` or ``*>``.
  Default weight of ``*<`` is 0.5. Default weight of ``*>`` is 2.0.

  We can specify score quantity by ``*<${score_quantity}`` and ``*>${score_quantity}``.
  Then, if we specify ``*<${score_quantity}``, a plus or minus sign of ``${score_quantity}`` is reverse.

  For example, ``'content @ "Groonga" *<2.5 query("content", "MySQL")'`` is as below.

    1. Extract records that match ``'content @ "Groonga"`` and ``content @ "Mroonga"'``.
    2. Add a score as below.

       a. Calculate the score of ``'content @ "Groonga"``.
       b. Calculate the score of ``query("content", "MySQL")``.
       c. b's score multiplied by -2.5 by ``*<``.
       d. The socre of this record is a + b
          Therefore, if a's socre is 1 and b's score is 1, the score of this record is 1 + (1 * -2.5) = -1.5.

  In particular, the following query adjust the score of match records by the following condition( ``'content @ "Groonga" *<2.5 query("content", "Mroonga")'`` ).

    .. code-block::

       table_create Memos TABLE_NO_KEY
       column_create Memos content COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerAuto
       column_create Terms index COLUMN_INDEX|WITH_POSITION Memos content

       load --table Memos
       [
       {"content": "Groonga is a full text search engine."},
       {"content": "Rroonga is the Ruby bindings of Groonga."},
       {"content": "Mroonga is a MySQL storage engine based of Groonga."}
       ]

       select Memos \
         --command_version 3 \
         --filter 'content @ "Groonga" *<2.5 query("content", "Mroonga")' \
         --output_columns 'content, _score' \
         --sort_keys -_score,_id
       {
         "header": {
           "return_code": 0,
           "start_time": 1624605205.641078,
           "elapsed_time": 0.002965450286865234
         },
         "body": {
           "n_hits": 3,
           "columns": [
             {
               "name": "content",
               "type": "ShortText"
             },
             {
               "name": "_score",
               "type": "Float"
             }
           ],
           "records": [
             [
               "Groonga is a full text search engine.",
               1.0
             ],
             [
               "Rroonga is the Ruby bindings of Groonga.",
               1.0
             ],
             [
               "Mroonga is a MySQL storage engine based of Groonga.",
               -1.5
             ]
           ]
         }
       }

* [:doc:`reference/log`] Added support for outputting to stdout and stderr.

  [:ref:`process-log`] and [:ref:`query-log`] supported　output to stdout and stderr.

    * If we specify as ``--log-path -``, ``--query-log-path -``, Groonga output log to stdout.
    * If we specify as ``--log-path +``, ``--query-log-path +``, Groonga output log to stderr.

  [:ref:`process-log`] is for all of Groonga works.
  [:ref:`query-log`] is just for query processing.

  This feature is useful when we execute Groonga on Docker.
  Docker has the feature that records stdout and stderr in standard.
  Therefore, we don't need to login into the environment of Docker to get Groonga's log.

  For example, this feature is useful as he following case.

    * If we want to analyze slow queries of Groonga on Docker.

      If we specify ``--query-log-path -`` when startup Groonga, we can analyze slow queries by only execution the following commands.

        * ``docker logs ${container_name} | groonga-query-log-analyze``

  By this, we can analyze slow query with the query log that output from Groonga on Docker simply.

* [Documentation] Filled missing documentation of ``string_substring``. [GitHub#1209][Patched by Takashi Hashida]

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

* ``*<`` and ``*>`` only valid when we use ``query()`` the right side of filter condition.
  If we specify as below, ``*<`` and ``*>`` work as ``&&``.

    * ``'content @ "Groonga" *< content @ "Mroonga"'``

Thanks
------

* Takashi Hashida

.. _release-11-0-3:

Release 11.0.3 - 2021-05-29
===========================

Improvements
------------

* [:doc:`reference/functions/query`] Added support for ignoring ``TokenFilterStem`` by the query.

  * ``TokenFilterStem`` can search by using a stem.
    For example, all of ``develop``, ``developing``, ``developed`` and ``develops`` tokens are stemmed as ``develop``.
    So we can find ``develop``, ``developing`` and ``developed`` by ``develops`` query.

  * In this release, we are able to search without ``TokenFilterStem`` in only a specific query as below.

    .. code-block::

       plugin_register token_filters/stem

       table_create Memos TABLE_NO_KEY
       column_create Memos content COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerAuto \
         --token_filters 'TokenFilterStem("keep_original", true)'
       column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content

       load --table Memos
       [
       {"content": "I develop Groonga"},
       {"content": "I'm developing Groonga"},
       {"content": "I developed Groonga"}
       ]

       select Memos \
         --match_columns content \
         --query '"developed groonga"' \
         --query_options '{"TokenFilterStem.enable": false}'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "content",
                 "ShortText"
               ]
             ],
             [
               3,
               "I developed Groonga"
             ]
           ]
         ]
       ]

  * This feature is useful when users want to search by a stemmed word generally but users sometimes want to search by a exact (not stemmed) word as below.

    * If Groonga returns many results when searching by a stemmed word.
    * If ``TokenFilterStem`` returns the wrong result of stemming.
    * If we want to find only records that have an exact (not stemmed) word.

* [:doc:`reference/functions/query`] Added support for ignoring ``TokenFilterStopWord`` by the query.

  * ``TokenFilterStopWord`` searched without stop word that we registered beforehand.
    It uses for reducing noise of search by ignoring frequently word (e.g., ``and``, ``is``, and so on.).
  * However, we sometimes want to search include these words only a specific query. In this release, we are able to search without ``TokenFilterStopWord`` in only a specific query as below.

    .. code-block::

       plugin_register token_filters/stop_word

       table_create Memos TABLE_NO_KEY
       column_create Memos content COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerAuto \
         --token_filters TokenFilterStopWord
       column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
       column_create Terms is_stop_word COLUMN_SCALAR Bool

       load --table Terms
       [
       {"_key": "and", "is_stop_word": true}
       ]

       load --table Memos
       [
       {"content": "Hello"},
       {"content": "Hello and Good-bye"},
       {"content": "Good-bye"}
       ]

       select Memos \
         --match_columns content \
         --query "Hello and" \
         --query_options '{"TokenFilterStopWord.enable": false}' \
         --match_escalation_threshold -1 \
         --sort_keys -_score
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "content",
                 "ShortText"
               ]
             ],
             [
               2,
               "Hello and Good-bye"
             ]
           ]
         ]
       ]

  * In the above example, we specify ``TokenFilterStopWord.enable`` by using ``--query-options``, but we also specify it by using ``{"options": {"TokenFilterStopWord.enable": false}}`` as below.

    .. code-block::

       plugin_register token_filters/stop_word

       table_create Memos TABLE_NO_KEY
       column_create Memos content COLUMN_SCALAR ShortText

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerAuto \
         --token_filters TokenFilterStopWord
       column_create Terms memos_content COLUMN_INDEX|WITH_POSITION Memos content
       column_create Terms is_stop_word COLUMN_SCALAR Bool

       load --table Terms
       [
       {"_key": "and", "is_stop_word": true}
       ]

       load --table Memos
       [
       {"content": "Hello"},
       {"content": "Hello and Good-bye"},
       {"content": "Good-bye"}
       ]

       select Memos \
         --filter 'query("content", \
                         "Hello and", \
                         {"options": {"TokenFilterStopWord.enable": false}})' \
         --match_escalation_threshold -1 \
         --sort_keys -_score
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "content",
                 "ShortText"
               ]
             ],
             [
               2,
               "Hello and Good-bye"
             ]
           ]
         ]
       ]

  * This feature is useful if that Groonga can't return results correctly if we don't search by keywords include commonly used words (e.g., if a search for a song title, a shop name, and so on.).

* [:doc:`/reference/normalizers`][NormalizerNFKC] Added a new option ``remove_new_line``.

  * If we want to normalize the key of a table that stores data, we set a normalizer to it.
    However, normally, normalizers remove a new line.
  * Groonga can't handle a key that is only a new line.
  * We can register data that is only a new line as key by this option.

* [:doc:`/reference/functions/string_slice`] Added a new function ``string_slice()``. [Github#1177][Patched by Takashi Hashida]

  * ``string_slice()`` extracts a substring of a string.
  * To enable this function, we need to register ``functions/string`` plugin.
  * We can use two different extraction methods depending on the arguments as below.

    * Extraction by position::

         plugin_register functions/string
         table_create Memos TABLE_HASH_KEY ShortText

         load --table Memos
         [
           {"_key": "Groonga"}
         ]
         select Memos --output_columns '_key, string_slice(_key, 2, 3)'
         [
           [
             0,
             1337566253.89858,
             0.000355720520019531
           ],
           [
             [
               [
                 1
               ],
               [
                 [
                   "_key",
                   "ShortText"
                 ],
                 [
                   "string_slice",
                   null
                 ]
               ],
               [
                 "Groonga",
                 "oon"
               ]
             ]
           ]
         ]

    * Extraction by regular expression::

         plugin_register functions/string
         table_create Memos TABLE_HASH_KEY ShortText

         load --table Memos
         [
           {"_key": "Groonga"}
         ]
         select Memos --output_columns '_key, string_slice(_key, "(Gro+)(.*)", 2)'
         [
           [p
             0,
             1337566253.89858,
             0.000355720520019531
           ],
           [
             [
               [
                 1
               ],
               [
                 [
                   "_key",
                   "ShortText"
                 ],
                 [
                   "string_slice",
                   null
                 ]
               ],
               [
                 "Groonga",
                 "nga"
               ]
             ]
           ]
         ]

* [:doc:`/install/ubuntu`] Dropped support for Ubuntu 16.04 LTS (Xenial Xerus).

* Added EditorConfig for Visual Studio. [GitHub#1191][Patched by Takashi Hashida]

  * Most settings are for Visual Studio only.

* [httpd] Updated bundled nginx to 1.20.1.

  * Contains security fix of CVE-2021-23017.

Fixes
-----

* Fixed a bug that Groonga may not have returned a result of a search query if we sent many search queries when tokenizer, normalizer, or token_filters that support options were used.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list.

Thanks
------

* Takashi Hashida

.. _release-11-0-2:

Release 11.0.2 - 2021-05-10
===========================

Improvements
------------

* [Documentation] Removed a reference about ``ruby_load`` command. [GitHub#1172][Patched by Anthony M. Cook]

  * Because this command has already deleted.

* [:doc:`/install/debian`] Added support for Debian 11(Bullseye).

* [:doc:`reference/commands/select`] Added support for ``--post_filter``.

  * We can use ``post_filter`` to filter by ``filtered`` stage dynamic columns as below.

    .. code-block::

       table_create Items TABLE_NO_KEY
       column_create Items price COLUMN_SCALAR UInt32

       load --table Items
       [
       {"price": 100},
       {"price": 150},
       {"price": 200},
       {"price": 250},
       {"price": 300}
       ]

       select Items \
         --filter "price >= 150" \
         --columns[price_with_tax].stage filtered \
         --columns[price_with_tax].type UInt32 \
         --columns[price_with_tax].flags COLUMN_SCALAR \
         --columns[price_with_tax].value "price * 1.1" \
         --post_filter "price_with_tax <= 250"
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               2
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "price_with_tax",
                 "UInt32"
               ],
               [
                 "price",
                 "UInt32"
               ]
             ],
             [
               2,
               165,
               150
             ],
             [
               3,
               220,
               200
             ]
           ]
         ]
       ]

* [:doc:`reference/commands/select`] Added support for ``--slices[].post_filter``.

  * We can use ``post_filter`` to filter by ``--slices[].filter`` as below.

    .. code-block::

        table_create Items TABLE_NO_KEY
        column_create Items price COLUMN_SCALAR UInt32

        load --table Items
        [
        {"price": 100},
        {"price": 200},
        {"price": 300},
        {"price": 1000},
        {"price": 2000},
        {"price": 3000}
        ]

        select Items \
          --slices[expensive].filter 'price >= 1000' \
          --slices[expensive].post_filter 'price < 3000'
        [
          [
            0,
            0.0,
            0.0
          ],
          [
            [
              [
                6
              ],
              [
                [
                  "_id",
                  "UInt32"
                ],
                [
                  "price",
                  "UInt32"
                ]
              ],
              [
                1,
                100
              ],
              [
                2,
                200
              ],
              [
                3,
                300
              ],
              [
                4,
                1000
              ],
              [
                5,
                2000
              ],
              [
                6,
                3000
              ]
            ],
            {
              "expensive": [
                [
                  2
                ],
                [
                  [
                    "_id",
                    "UInt32"
                  ],
                  [
                    "price",
                    "UInt32"
                  ]
                ],
                [
                  4,
                  1000
                ],
                [
                  5,
                  2000
                ]
              ]
            }
          ]
        ]


* [:doc:`reference/commands/select`] Added support for describing expression into ``--sort_keys``.

  * We can describe the expression into ``--sort_keys``.

    * If nonexistent keys into expression as a ``--sort_keys``, they are ignored and outputted warns into a log.

  * By this, for example, we can specify a value of an element of ``VECTOR COLUMN`` to ``--sort_keys``. And we can sort a result with it.
  * We can sort a result with an element of ``VECTOR COLUMN`` even if the before version by using dynamic column.
    However, we can sort a result with an element of ``VECTOR COLUMN`` without using dynamic column by this feature.

    .. code-block::

       table_create Values TABLE_NO_KEY
       column_create Values numbers COLUMN_VECTOR Int32
       load --table Values
       [
       {"numbers": [127, 128, 129]},
       {"numbers": [126, 255]},
       {"numbers": [128, -254]}
       ]
       select Values --sort_keys 'numbers[1]' --output_columns numbers
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               3
             ],
             [
               [
                 "numbers",
                 "Int32"
               ]
             ],
             [
               [
                 128,
                 -254
               ]
             ],
             [
               [
                 127,
                 128,
                 129
               ]
             ],
             [
               [
                 126,
                 255
               ]
             ]
           ]
         ]
       ]

* [:doc:`/reference/token_filters`] Added support for multiple token filters with options.

  * We can specify multiple token filters with options like ``--token_filters 'TokenFilterStopWord("column", "ignore"), TokenFilterNFKC130("unify_kana", true)'``. [Github#mroonga/mroonga#399][Reported by MASUDA Kazuhiro]

* [:doc:`reference/functions/query`] Added support a dynamic column of ``result_set`` stage with complex expression.

  * Complex expression is that it needs temporary result sets internally like a following expression.

    .. code-block::

       '(true && query("name * 10", "ali", {"score_column": "ali_score"})) || \
        (true && query("name * 2", "li", {"score_column": "li_score"}))'

    * In the above expressions, the temporary result sets are used to store the result of evaluating the ``true``.
    * Therefore, for example, in the following expression, we can use a value of dynamic column of ``result_set`` stage in expression. Because temporary result sets internally are needless as below expression.

      .. code-block::

         '(query("name * 10", "ali", {"score_column": "ali_score"})) || \
          (query("name * 2", "li", {"score_column": "li_score"}))'

  * In this release, for example, we can set a value to ``li_score`` as below. (The value of ``li_score`` had been ``0`` in before version. Because the second expression could not get dynamic column.)

    .. code-block::

       table_create Users TABLE_NO_KEY
       column_create Users name COLUMN_SCALAR ShortText

       table_create Lexicon TABLE_HASH_KEY ShortText \
         --default_tokenizer TokenBigramSplitSymbolAlphaDigit \
         --normalizer NormalizerAuto
       column_create Lexicon users_name COLUMN_INDEX|WITH_POSITION Users name

       load --table Users
       [
       {"name": "Alice"},
       {"name": "Alisa"},
       {"name": "Bob"}
       ]

       select Users \
         --columns[ali_score].stage result_set \
         --columns[ali_score].type Float \
         --columns[ali_score].flags COLUMN_SCALAR \
         --columns[li_score].stage result_set \
         --columns[li_score].type Float \
         --columns[li_score].flags COLUMN_SCALAR \
         --output_columns name,_score,ali_score,li_score \
         --filter '(true && query("name * 10", "ali", {"score_column": "ali_score"})) || \
                   (true && query("name * 2", "li", {"score_column": "li_score"}))'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               2
             ],
             [
               [
                 "name",
                 "ShortText"
               ],
               [
                 "_score",
                 "Int32"
               ],
               [
                 "ali_score",
                 "Float"
               ],
               [
                 "li_score",
                 "Float"
               ]
             ],
             [
               "Alice",
               14,
               10.0,
               2.0
             ],
             [
               "Alisa",
               14,
               10.0,
               2.0
             ]
           ]
         ]
       ]

  * We also supported a dynamic vector column of ``result_set`` stage as below.

    .. code-block::

       table_create Users TABLE_NO_KEY
       column_create Users name COLUMN_SCALAR ShortText

       table_create Lexicon TABLE_HASH_KEY ShortText \
         --default_tokenizer TokenBigramSplitSymbolAlphaDigit \
         --normalizer NormalizerAuto
       column_create Lexicon users_name COLUMN_INDEX|WITH_POSITION Users name

       load --table Users
       [
       {"name": "Alice"},
       {"name": "Alisa"},
       {"name": "Bob"}
       ]

       select Users \
         --columns[tags].stage result_set \
         --columns[tags].type ShortText \
         --columns[tags].flags COLUMN_VECTOR \
         --output_columns name,tags \
         --filter '(true && query("name", "al", {"tags": ["al"], "tags_column": "tags"})) || \
                   (true && query("name", "sa", {"tags": ["sa"], "tags_column": "tags"}))'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               2
             ],
             [
               [
                 "name",
                 "ShortText"
               ],
               [
                 "tags",
                 "ShortText"
               ]
             ],
             [
               "Alice",
               [
                 "al"
               ]
             ],
             [
               "Alisa",
               [
                 "al",
                 "sa"
               ]
             ]
           ]
         ]
       ]

    * If we use a dynamic vector column, the storing values are appended values of each element.

* [:doc:`/install/ubuntu`] Added support for Ubuntu 21.04 (Hirsute Hippo).

* [httpd] Updated bundled nginx to 1.19.10.

Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

* [The browser based administration tool] Currently, Groonga has a bug that a search query that is inputted to non-administration mode is sent even if we input checks to the checkbox for the administration mode of a record list. [Github#1186][Reported by poti]

Thanks
------

* Anthony M. Cook

* MASUDA Kazuhiro

* poti

.. _release-11-0-1:

Release 11.0.1 - 2021-03-31
===========================

Improvements
------------

* [:doc:`/install/debian`] Added support for a ARM64 package.

* [:doc:`reference/commands/select`] Added support for customizing adjust weight every key word.

  * We need to specify ``<`` or ``>`` to all keywords to adjust scores until now.
    Because the default adjustment of weight (6 or 4) is larger than the default score (1).

    * Therefore, for example, "A"'s weight is 1 and "B"'s weight is 4 in ``A <B``.
      Decremented "B"'s weight (4) is larger than not decremented "A"'s weight (1).
      This is not works as expected.
      we need to specify ``>A <B`` to use smaller weight than "A" for "B".
      "A"'s weight is 6 and "B"'s weight is 4 in ``>A <B``.

  * We can customize adjustment of weight every key word by only specifying ``<${WEIGHT}`` or ``>${WEIGHT}`` to target keywords since this release.
    For example, "A"'s weight is 1 and "B"'s weight is 0.9 in ``A <0.1B`` ("B"'s weight decrement 0.1).

  * However, note that these forms ( ``>${WEIGHT}...``, ``<${WEIGHT}...``, and ``~${WEIGHT}...`` ) are incompatible.

* [:doc:`reference/commands/select`] Added support for outputting ``Float`` and ``Float32`` value in Apache Arrow format.

  * For example, Groonga output as below.

  .. code-block::

     table_create Data TABLE_NO_KEY
     column_create Data float COLUMN_SCALAR Float

     load --table Data
     [
     {"float": 1.1}
     ]

     select Data \
       --command_version 3 \
       --output_type apache-arrow

       return_code: int32
       start_time: timestamp[ns]
       elapsed_time: double
       -- metadata --
       GROONGA:data_type: metadata
       	return_code	               start_time	elapsed_time
       0	          0	1970-01-01T09:00:00+09:00	    0.000000
       ========================================
       _id: uint32
       float: double
       -- metadata --
       GROONGA:n_hits: 1
       	_id	     float
       0	  1	  1.100000

* [:doc:`reference/commands/select`] Added support for getting a reference destination data via index column when we output a result.

  * Until now, Groonga had returned involuntary value when we specified output value like ``index_column.xxx``.
    For example, A value of ``--columns[tags].value purchases.tag`` was ``["apple",["many"]],["banana",["man"]],["cacao",["man"]]`` in the following example. In this case, the expected values was ``["apple",["man","many"]],["banana",["man"]],["cacao",["woman"]]``.
    In this release, we can get a correct reference destination data via index column as below.

    .. code-block::

      table_create Products TABLE_PAT_KEY ShortText

      table_create Purchases TABLE_NO_KEY
      column_create Purchases product COLUMN_SCALAR Products
      column_create Purchases tag COLUMN_SCALAR ShortText

      column_create Products purchases COLUMN_INDEX Purchases product

      load --table Products
      [
      {"_key": "apple"},
      {"_key": "banana"},
      {"_key": "cacao"}
      ]

      load --table Purchases
      [
      {"product": "apple",  "tag": "man"},
      {"product": "banana", "tag": "man"},
      {"product": "cacao",  "tag": "woman"},
      {"product": "apple",  "tag": "many"}
      ]

      select Products \
        --columns[tags].stage output \
        --columns[tags].flags COLUMN_VECTOR \
        --columns[tags].type ShortText \
        --columns[tags].value purchases.tag \
        --output_columns _key,tags
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              3
            ],
            [
              [
                "_key",
                "ShortText"
              ],
              [
                "tags",
                "ShortText"
              ]
            ],
            [
              "apple",
              [
                "man",
                "many"
              ]
            ],
            [
              "banana",
              [
                "man"
              ]
            ],
            [
              "cacao",
              [
                "woman"
              ]
            ]
          ]
        ]
      ]

* [:doc:`reference/commands/select`] Added support for specifying index column directly as a part of nested index.

  * We can search source table after filtering by using ``index_column.except_source_column``.
    For example, we specify ``comments.content`` when searching in the following example.
    In this case, at first, this query execute full text search from ``content`` column of ``Commentts`` table, then fetch the records of Articles table which refers to already searched records of Comments table.

    .. code-block::

       table_create Articles TABLE_HASH_KEY ShortText

       table_create Comments TABLE_NO_KEY
       column_create Comments article COLUMN_SCALAR Articles
       column_create Comments content COLUMN_SCALAR ShortText

       column_create Articles content COLUMN_SCALAR Text
       column_create Articles comments COLUMN_INDEX Comments article

       table_create Terms TABLE_PAT_KEY ShortText \
         --default_tokenizer TokenBigram \
         --normalizer NormalizerNFKC130
       column_create Terms articles_content COLUMN_INDEX|WITH_POSITION \
         Articles content
       column_create Terms comments_content COLUMN_INDEX|WITH_POSITION \
         Comments content

       load --table Articles
       [
       {"_key": "article-1", "content": "Groonga is fast!"},
       {"_key": "article-2", "content": "Groonga is useful!"},
       {"_key": "article-3", "content": "Mroonga is fast!"}
       ]

       load --table Comments
       [
       {"article": "article-1", "content": "I'm using Groonga too!"},
       {"article": "article-3", "content": "I'm using Mroonga!"},
       {"article": "article-1", "content": "I'm using PGroonga!"}
       ]

       select Articles --match_columns comments.content --query groonga \
         --output_columns "_key, _score, comments.content
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               1
             ],
             [
               [
                 "_key",
                 "ShortText"
               ],
               [
                 "_score",
                 "Int32"
               ],
               [
                 "comments.content",
                 "ShortText"
               ]
             ],
             [
               "article-1",
               1,
               [
                 "I'm using Groonga too!",
                 "I'm using PGroonga!"
               ]
             ]
           ]
         ]
       ]

* [:doc:`/reference/commands/load`] Added support for loading reference vector with inline object literal.

  * For example, we can load data like ``"key" : "[ { "key" : "value", ..., "key" : "value" } ]"`` as below.

    .. code-block::

      table_create Purchases TABLE_NO_KEY
      column_create Purchases item COLUMN_SCALAR ShortText
      column_create Purchases price COLUMN_SCALAR UInt32

      table_create Settlements TABLE_HASH_KEY ShortText
      column_create Settlements purchases COLUMN_VECTOR Purchases
      column_create Purchases settlements_purchases COLUMN_INDEX Settlements purchases

      load --table Settlements
      [
      {
        "_key": "super market",
        "purchases": [
           {"item": "apple", "price": 100},
           {"item": "milk",  "price": 200}
        ]
      },
      {
        "_key": "shoes shop",
        "purchases": [
           {"item": "sneakers", "price": 3000}
        ]
      }
      ]

  * It makes easier to add JSON data into reference columns by this feature.
  * Currently, this feature only support with JSON input.

* [:doc:`/reference/commands/load`] Added support for loading reference vector from JSON text.

  * We can load data to reference vector from source table with JSON text as below.

    .. code-block::

      table_create Purchases TABLE_HASH_KEY ShortText
      column_create Purchases item COLUMN_SCALAR ShortText
      column_create Purchases price COLUMN_SCALAR UInt32

      table_create Settlements TABLE_HASH_KEY ShortText
      column_create Settlements purchases COLUMN_VECTOR Purchases

      column_create Purchases settlements_purchases COLUMN_INDEX Settlements purchases

      load --table Settlements
      [
      {
        "_key": "super market",
        "purchases": "[{\"_key\": \"super market-1\", \"item\": \"apple\", \"price\": 100}, {\"_key\": \"super market-2\", \"item\": \"milk\",  \"price\": 200}]"
      },
      {
        "_key": "shoes shop",
        "purchases": "[{\"_key\": \"shoes shop-1\", \"item\": \"sneakers\", \"price\": 3000}]"
      }
      ]

      dump \
        --dump_plugins no \
        --dump_schema no
      load --table Purchases
      [
      ["_key","item","price"],
      ["super market-1","apple",100],
      ["super market-2","milk",200],
      ["shoes shop-1","sneakers",3000]
      ]

      load --table Settlements
      [
      ["_key","purchases"],
      ["super market",["super market-1","super market-2"]],
      ["shoes shop",["shoes shop-1"]]
      ]

      column_create Purchases settlements_purchases COLUMN_INDEX Settlements purchases

  * Currently, this feature doesn't support nested reference record.

* [Windows] Added support for UNIX epoch for ``time_classify_*`` functions.

  * Groonga handles timestamps on local time. Therefore, for example, if we input the UNIX epoch in Japan, inputting time is 9 hours behind the UNIX epoch.

  * The Windows API outputs an error when we input the time before the UNIX epoch.

  * We can use the UNIX epoch in ``time_classify_*`` functions as below in this release.

    .. code-block::

       plugin_register functions/time

       table_create Timestamps TABLE_PAT_KEY Time
       load --table Timestamps
       [
       {"_key": 0},
       {"_key": "2016-05-06 00:00:00.000001"},
       {"_key": "2016-05-06 23:59:59.999999"},
       {"_key": "2016-05-07 00:00:00.000000"},
       {"_key": "2016-05-07 00:00:00.000001"},
       {"_key": "2016-05-08 23:59:59.999999"},
       {"_key": "2016-05-08 00:00:00.000000"}
       ]

       select Timestamps \
         --sortby _id \
         --limit -1 \
         --output_columns '_key, time_classify_day_of_week(_key)'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               7
             ],
             [
               [
                 "_key",
                 "Time"
               ],
               [
                 "time_classify_day_of_week",
                 null
               ]
             ],
             [
               0.0,
               4
             ],
             [
               1462460400.000001,
               5
             ],
             [
               1462546799.999999,
               5
             ],
             [
               1462546800.0,
               6
             ],
             [
               1462546800.000001,
               6
             ],
             [
               1462719599.999999,
               0
             ],
             [
               1462633200.0,
               0
             ]
           ]
         ]
       ]

* [:doc:`reference/functions/query_parallel_or`] Added a new function for processing queries in parallel.

  * ``query_parallel_or`` requires Apache Arrow for processing queries in parallel.
    If it does not enable, ``query_parallel_or`` processes queries in sequence.

  * ``query_parallel_or`` processes combination of ``match_columns`` and ``query_string`` in parallel.

  * Syntax of ``query_parallel_or`` is as follow::

      query_parallel_or(match_columns, query_string1,
                                       query_string2,
                                       .
                                       .
                                       .
                                       query_stringN,
                                       {"option": "value", ...})

* [:doc:`reference/commands/select`] Added support for ignoring nonexistent sort keys.

  * Groonga had been outputted error when we specified nonexistent sort keys until now.
    However, Groonga ignore nonexistent sort keys since this release. (Groonga doesn't output error.)
  * This feature implements for consistency.
    Because we just ignore invalid values in ``output_columns`` and most of invalid values in ``sort_keys``.

* [:doc:`reference/commands/select`] Added support for ignoring nonexistent tables in ``drilldowns[].table``. [GitHub#1169][Reported by naoa]

  * Groonga had been outputted error when we specified nonexistent tables in ``drilldowns[].table`` until now.
    However, Groonga ignore nonexistent tables in ``drilldowns[].table`` since this release. (Groonga doesn't output error.)
  * This feature implements for consistency.
    Because we just ignore invalid values in ``output_columns`` and most of invalid values in ``sort_keys``.

* [httpd] Updated bundled nginx to 1.19.8.

Fixes
-----

* [:doc:`reference/commands/reference_acquire`] Fixed a bug that Groonga crash when a table's reference is acquired and a column is added to the table before auto release is happened.

  * Because the added column's reference isn't acquired but it's released on auto release.

* [Windows] Fixed a bug that one or more processes fail an output backtrace on SEGV when a new backtrace logging process starts when another backtrace logging process is running in another thread.


Known Issues
------------

* Currently, Groonga has a bug that there is possible that data is corrupt when we execute many additions, delete, and update data to vector column.

Thanks
------

* naoa

.. _release-11-0-0:

Release 11.0.0 - 2021-02-09
===========================

This is a major version up! But It keeps backward compatibility. We can upgrade to 11.0.0 without rebuilding database.

Improvements
------------

* [:doc:`reference/commands/select`] Added support for outputting values of scalar column and vector column via nested index.

  * The nested index is that has structure as below.

    .. code-block::

      table_create Products TABLE_PAT_KEY ShortText

      table_create Purchases TABLE_NO_KEY
      column_create Purchases product COLUMN_SCALAR Products
      column_create Purchases tag COLUMN_SCALAR ShortText

      column_create Products purchases COLUMN_INDEX Purchases product

  * The ``Products.purchases`` column is a index of ``Purchases.product`` column in the above example.
    Also, ``Purchases.product`` is a reference to ``Products`` table.

  * We had not got the correct search result when we search via nested index until now.
  * The result had been as follows until now. We can see that ``{"product": "apple",  "tag": "man"}`` is not output.

    .. code-block::

      table_create Products TABLE_PAT_KEY ShortText

      table_create Purchases TABLE_NO_KEY
      column_create Purchases product COLUMN_SCALAR Products
      column_create Purchases tag COLUMN_SCALAR ShortText

      column_create Products purchases COLUMN_INDEX Purchases product

      load --table Products
      [
      {"_key": "apple"},
      {"_key": "banana"},
      {"_key": "cacao"}
      ]

      load --table Purchases
      [
      {"product": "apple",  "tag": "man"},
      {"product": "banana", "tag": "man"},
      {"product": "cacao",  "tag": "woman"},
      {"product": "apple",  "tag": "many"}
      ]

      select Products \
        --output_columns _key,purchases.tag
      [
        [
          0,
          1612504193.380738,
          0.0002026557922363281
        ],
        [
          [
            [
              3
            ],
            [
              [
                "_key",
                "ShortText"
              ],
              [
                "purchases.tag",
                "ShortText"
              ]
            ],
            [
              "apple",
              "many"
            ],
            [
              "banana",
              "man"
            ],
            [
              "cacao",
              "man"
            ]
          ]
        ]
      ]

  * The result will be as follows from this release. We can see that ``{"product": "apple",  "tag": "man"}`` is output.

    .. code-block::

      select Products \
        --output_columns _key,purchases.tag
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              3
            ],
            [
              [
                "_key",
                "ShortText"
              ],
              [
                "purchases.tags",
                "Tags"
              ]
            ],
            [
              "apple",
              [
                [
                  "man",
                  "one"
                ],
                [
                  "child",
                  "many"
                ]
              ]
            ],
            [
              "banana",
              [
                [
                  "man",
                  "many"
                ]
              ]
            ],
            [
              "cacao",
              [
                [
                  "woman"
                ]
              ]
            ]
          ]
        ]
      ]

* [Windows] Dropped support for packages of Windows version that we had cross compiled by using MinGW on Linux.

  * Because there aren't probably many people who use that.
  * These above packages are that We had provided as below name until now.

    * ``groonga-x.x.x-x86.exe``
    * ``groonga-x.x.x-x86.zip``
    * ``groonga-x.x.x-x64.exe``
    * ``groonga-x.x.x-x86.zip``

  * From now on, we use the following packages for Windows.

    * ``groonga-latest-x86-vs2019-with-vcruntime.zip``
    * ``groonga-latest-x64-vs2019-with-vcruntime.zip``

  * If a system already has installed Microsoft Visual C++ Runtime Library, we suggest that we use the following packages.

    * ``groonga-latest-x86-vs2019.zip``
    * ``groonga-latest-x64-vs2019.zip``

Fixes
-----

* Fixed a bug that there is possible that index is corrupt when Groonga executes many additions, delete, and update information in it.

  * This bug occurs when we only execute many delete information from index.
    However, it doesn't occur when we only execute many additions information into index.

  * We can repair the index that is corrupt by this bug using reconstruction of it.

  * This bug doesn't detect unless we reference the broken index.
    Therefore, the index in our indexes may has already broken.

  * We can use [:doc:`reference/commands/index_column_diff`] command to confirm whether the index has already been broken or not.

.. _release-10-1-1:

Release 10.1.1 - 2021-01-25
===========================

Improvements
------------

* [:doc:`reference/commands/select`] Added support for outputting UInt64 value in Apache Arrow format.

* [:doc:`reference/commands/select`] Added support for outputting a number of hits in Apache Arrow format as below.

  .. code-block::

     -- metadata --
     GROONGA:n-hits: 10

* [:doc:`reference/functions/query`] Added support for optimization of "order by estimated size".

  * Normally, we can search at high speed when we first execute a condition which number of hits is a little.

    * The "B (There are few) && A (There are many)" faster than "A (There are many) && B (There are few)".

  * This is a known optimization. However we need reorder conditions by ourselves.
  * Groonga executes this reorder automatically by using optimization of "order by estimated size".
  * This optimization is valid by ``GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE=yes``.

* [:doc:`/reference/functions/between`] Improved performance by the following improvements.

  * Improved the accuracy of a decision whether the ``between()`` use sequential search or not.
  * Improved that we set a result of ``between()`` into a result set in bulk.

* [:doc:`reference/commands/select`] Improved performance for a prefix search.

  * For example, the performance of the following prefix search by using "*" improved.

    .. code-block::

       table_create Memos TABLE_PAT_KEY ShortText
       table_create Contents TABLE_PAT_KEY ShortText   --normalizer NormalizerAuto
       column_create Contents entries_key_index COLUMN_INDEX Memos _key

       load --table Memos
       [
       {"_key": "(groonga) Start to try!"},
       {"_key": "(mroonga) Installed"},
       {"_key": "(groonga) Upgraded!"}
       ]

       select \
         --table Memos \
         --match_columns _key \
         --query '\\(groonga\\)*'

* [:doc:`/reference/tokenizers`][TokenMecab] Improved performance for parallel construction fo token column. [GitHub#1158][Patched by naoa]

Fixes
-----

* [:doc:`reference/functions/sub_filter`] Fixed a bug that ``sub_filter`` doesn't work in ``slices[].filter``.

  * For example, the result of ``sub_filter`` was 0 records by the following query by this bug.

    .. code-block::

       table_create Users TABLE_HASH_KEY ShortText
       column_create Users age COLUMN_SCALAR UInt8

       table_create Memos TABLE_NO_KEY
       column_create Memos user COLUMN_SCALAR Users
       column_create Memos content COLUMN_SCALAR Text

       load --table Users
       [
       {"_key": "alice", "age": 9},
       {"_key": "bob",   "age": 29}
       ]

       load --table Memos
       [
       {"user": "alice", "content": "hello"},
       {"user": "bob",   "content": "world"},
       {"user": "alice", "content": "bye"},
       {"user": "bob",   "content": "yay"}
       ]

       select \
         --table Memos \
         --slices[adult].filter '_id > 1 && sub_filter(user, "age >= 18")'

* Fixed a bug that it is possible that we can't add data and Groonga crash when we repeat much addition of data and deletion of data against a hash table.

Thanks
------

* naoa

.. _release-10-1-0:

Release 10.1.0 - 2020-12-29
===========================

Improvements
------------

* [:doc:`/reference/functions/highlight_html`] Added support for removing leading full width spaces from highlight target. [PGroonga#GitHub#155][Reported by Hung Nguyen V]

  * Until now, leading full width spaces had also included in the target of highlight as below.

    .. code-block::

      table_create Entries TABLE_NO_KEY
      column_create Entries body COLUMN_SCALAR ShortText

      table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
      column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body

      load --table Entries
      [
      {"body": "Groonga　高速！"}
      ]

      select Entries --output_columns \
        --match_columns body --query '高速' \
        --output_columns 'highlight_html(body)'
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              1
            ],
            [
              [
                "highlight_html",
                null
              ]
            ],
            [
              "Groonga<span class=\"keyword\">　高速</span>！"
            ]
          ]
        ]
      ]

  * However, this is needless as the target of highlight.
    Therefore, in this release, ``highlight_html()`` removes leading full width spaces.

* [:doc:`reference/commands/status`] Added a new item ``features``.

  * We can display which Groonga's features are enabled as below.

    .. code-block::

      status --output_pretty yes
      [
        [
          0,
          0.0,
          0.0
        ],
        {
          "alloc_count": 361,
          "starttime": 1608087311,
          "start_time": 1608087311,
          "uptime": 35,
          "version": "10.1.0",
          "n_queries": 0,
          "cache_hit_rate": 0.0,
          "command_version": 1,
          "default_command_version": 1,
          "max_command_version": 3,
          "n_jobs": 0,
          "features": {
            "nfkc": true,
            "mecab": true,
            "message_pack": true,
            "mruby": true,
            "onigmo": true,
            "zlib": true,
            "lz4": false,
            "zstandard": false,
            "kqueue": false,
            "epoll": true,
            "poll": false,
            "rapidjson": false,
            "apache_arrow": false,
            "xxhash": false
          }
        }
      ]

* [:doc:`reference/commands/status`] Added a new item ``apache_arrow``.

  * We can display Apache Arrow version that Groonga use as below.

  .. code-block::

    [
      [
        0,
        1608088628.440753,
        0.0006628036499023438
      ],
      {
        "alloc_count": 360,
        "starttime": 1608088617,
        "start_time": 1608088617,
        "uptime": 11,
        "version": "10.0.9-39-g5a4c6f3",
        "n_queries": 0,
        "cache_hit_rate": 0.0,
        "command_version": 1,
        "default_command_version": 1,
        "max_command_version": 3,
        "n_jobs": 0,
        "features": {
          "nfkc": true,
          "mecab": true,
          "message_pack": true,
          "mruby": true,
          "onigmo": true,
          "zlib": true,
          "lz4": true,
          "zstandard": false,
          "kqueue": false,
          "epoll": true,
          "poll": false,
          "rapidjson": false,
          "apache_arrow": true,
          "xxhash": false
        },
        "apache_arrow": {
          "version_major": 2,
          "version_minor": 0,
          "version_patch": 0,
          "version": "2.0.0"
        }
      }
    ]

  * This item only display when Apache Arrow is valid in Groonga.

* [:doc:`/reference/window_function`] Added support for processing all tables at once even if target tables straddle a shard. (experimental)

  * If the target tables straddle a shard, the window function had processed each shard until now.

    * Therefore, if we used multiple group keys in windows functions, the value of the group keys from the second had to be one kind of value.
    * However, we can use multiple kind of values for it as below by this improvement.

      .. code-block::

        plugin_register sharding
        plugin_register functions/time

        table_create Logs_20170315 TABLE_NO_KEY
        column_create Logs_20170315 timestamp COLUMN_SCALAR Time
        column_create Logs_20170315 price COLUMN_SCALAR UInt32
        column_create Logs_20170315 item COLUMN_SCALAR ShortText

        table_create Logs_20170316 TABLE_NO_KEY
        column_create Logs_20170316 timestamp COLUMN_SCALAR Time
        column_create Logs_20170316 price COLUMN_SCALAR UInt32
        column_create Logs_20170316 item COLUMN_SCALAR ShortText

        table_create Logs_20170317 TABLE_NO_KEY
        column_create Logs_20170317 timestamp COLUMN_SCALAR Time
        column_create Logs_20170317 price COLUMN_SCALAR UInt32
        column_create Logs_20170317 item COLUMN_SCALAR ShortText

        load --table Logs_20170315
        [
        {"timestamp": "2017/03/15 10:00:00", "price": 1000, "item": "A"},
        {"timestamp": "2017/03/15 11:00:00", "price":  900, "item": "A"},
        {"timestamp": "2017/03/15 12:00:00", "price":  300, "item": "B"},
        {"timestamp": "2017/03/15 13:00:00", "price":  200, "item": "B"}
        ]

        load --table Logs_20170316
        [
        {"timestamp": "2017/03/16 10:00:00", "price":  530, "item": "A"},
        {"timestamp": "2017/03/16 11:00:00", "price":  520, "item": "B"},
        {"timestamp": "2017/03/16 12:00:00", "price":  110, "item": "A"},
        {"timestamp": "2017/03/16 13:00:00", "price":  410, "item": "A"},
        {"timestamp": "2017/03/16 14:00:00", "price":  710, "item": "B"}
        ]

        load --table Logs_20170317
        [
        {"timestamp": "2017/03/17 10:00:00", "price":  800, "item": "A"},
        {"timestamp": "2017/03/17 11:00:00", "price":  400, "item": "B"},
        {"timestamp": "2017/03/17 12:00:00", "price":  500, "item": "B"},
        {"timestamp": "2017/03/17 13:00:00", "price":  300, "item": "A"}
        ]

        table_create Times TABLE_PAT_KEY Time
        column_create Times logs_20170315 COLUMN_INDEX Logs_20170315 timestamp
        column_create Times logs_20170316 COLUMN_INDEX Logs_20170316 timestamp
        column_create Times logs_20170317 COLUMN_INDEX Logs_20170317 timestamp

        logical_range_filter Logs \
          --shard_key timestamp \
          --filter 'price >= 300' \
          --limit -1 \
          --columns[offsetted_timestamp].stage filtered \
          --columns[offsetted_timestamp].type Time \
          --columns[offsetted_timestamp].flags COLUMN_SCALAR \
          --columns[offsetted_timestamp].value 'timestamp - 39600000000' \
          --columns[offsetted_day].stage filtered \
          --columns[offsetted_day].type Time \
          --columns[offsetted_day].flags COLUMN_SCALAR \
          --columns[offsetted_day].value 'time_classify_day(offsetted_timestamp)' \
          --columns[n_records_per_day_and_item].stage filtered \
          --columns[n_records_per_day_and_item].type UInt32 \
          --columns[n_records_per_day_and_item].flags COLUMN_SCALAR \
          --columns[n_records_per_day_and_item].value 'window_count()' \
          --columns[n_records_per_day_and_item].window.group_keys 'offsetted_day,item' \
          --output_columns "_id,time_format_iso8601(offsetted_day),item,n_records_per_day_and_item"
        [
          [
            0,
            0.0,
            0.0
          ],
          [
            [
              [
                "_id",
                "UInt32"
              ],
              [
                "time_format_iso8601",
                null
              ],
              [
                "item",
                "ShortText"
              ],
              [
                "n_records_per_day_and_item",
                "UInt32"
              ]
            ],
            [
              1,
              "2017-03-14T00:00:00.000000+09:00",
              "A",
              1
            ],
            [
              2,
              "2017-03-15T00:00:00.000000+09:00",
              "A",
              2
            ],
            [
              3,
              "2017-03-15T00:00:00.000000+09:00",
              "B",
              1
            ],
            [
              1,
              "2017-03-15T00:00:00.000000+09:00",
              "A",
              2
            ],
            [
              2,
              "2017-03-16T00:00:00.000000+09:00",
              "B",
              2
            ],
            [
              4,
              "2017-03-16T00:00:00.000000+09:00",
              "A",
              2
            ],
            [
              5,
              "2017-03-16T00:00:00.000000+09:00",
              "B",
              2
            ],
            [
              1,
              "2017-03-16T00:00:00.000000+09:00",
              "A",
              2
            ],
            [
              2,
              "2017-03-17T00:00:00.000000+09:00",
              "B",
              2
            ],
            [
              3,
              "2017-03-17T00:00:00.000000+09:00",
              "B",
              2
            ],
            [
              4,
              "2017-03-17T00:00:00.000000+09:00",
              "A",
              1
            ]
          ]
        ]

  * This feature requires Apache Arrow 3.0.0 that is not released yet.

* Added support for sequential search against reference column.

  * This feature is only used if an index search will match many records and the current result set is enough small.

    * Because the sequential search is faster than the index search in the above case.

  * It is invalid by default.
  * It is valid if we set ``GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_REFERENCE`` environment variable.

  * ``GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_REFERENCE`` environment variable is a threshold to switch the sequential search from the index search.

    * For example, if we set ``GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_REFERENCE=0.7`` as below, if the number of records for the result set less than 70 % of total records, a search is executed by a sequential search.

      .. code-block ::

        $ export GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_REFERENCE=0.7

        table_create Tags TABLE_HASH_KEY ShortText
        table_create Memos TABLE_HASH_KEY ShortText
        column_create Memos tag COLUMN_SCALAR Tags

        load --table Memos
        [
        {"_key": "Rroonga is fast!", "tag": "Rroonga"},
        {"_key": "Groonga is fast!", "tag": "Groonga"},
        {"_key": "Mroonga is fast!", "tag": "Mroonga"},
        {"_key": "Groonga sticker!", "tag": "Groonga"},
        {"_key": "Groonga is good!", "tag": "Groonga"}
        ]

        column_create Tags memos_tag COLUMN_INDEX Memos tag

        select Memos --query '_id:>=3 tag:@Groonga' --output_columns _id,_score,_key,tag
        [
          [
            0,
            0.0,
            0.0
          ],
          [
            [
              [
                2
              ],
              [
                [
                  "_id",
                  "UInt32"
                ],
                [
                  "_score",
                  "Int32"
                ],
                [
                  "_key",
                  "ShortText"
                ],
                [
                  "tag",
                  "Tags"
                ]
              ],
              [
                4,
                2,
                "Groonga sticker!",
                "Groonga"
              ],
              [
                5,
                2,
                "Groonga is good!",
                "Groonga"
              ]
            ]
          ]
        ]

* [tokenizers] Added support for the token column into ``TokenDocumentVectorTFIDF`` and ``TokenDocumentVectorBM25``.

  * If there is the token column that has the same source as the index column, these tokenizer use the token id of the token column.

    * The token column has already had data that has already finished tokenize.
    * Therefore, these tokenizer are improved performance by using a token column.

  * For example, we can use this feature by making a token column named ``content_tokens`` as below.

    .. code-block::

      table_create Memos TABLE_NO_KEY
      column_create Memos content COLUMN_SCALAR Text

      load --table Memos
      [
      {"content": "a b c a"},
      {"content": "c b c"},
      {"content": "b b a"},
      {"content": "a c c"},
      {"content": "a"}
      ]

      table_create Tokens TABLE_PAT_KEY ShortText \
        --normalizer NormalizerNFKC121 \
        --default_tokenizer TokenNgram
      column_create Tokens memos_content COLUMN_INDEX|WITH_POSITION Memos content

      column_create Memos content_tokens COLUMN_VECTOR Tokens content

      table_create DocumentVectorBM25 TABLE_HASH_KEY Tokens \
        --default_tokenizer \
          'TokenDocumentVectorBM25("index_column", "memos_content", \
                                   "df_column", "df")'
      column_create DocumentVectorBM25 df COLUMN_SCALAR UInt32

      column_create Memos content_feature COLUMN_VECTOR|WITH_WEIGHT|WEIGHT_FLOAT32 \
        DocumentVectorBM25 content

      select Memos
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              5
            ],
            [
              [
                "_id",
                "UInt32"
              ],
              [
                "content",
                "Text"
              ],
              [
                "content_feature",
                "DocumentVectorBM25"
              ],
              [
                "content_tokens",
                "Tokens"
              ]
            ],
            [
              1,
              "a b c a",
              {
                "a": 0.5095787,
                "b": 0.6084117,
                "c": 0.6084117
              },
              [
                "a",
                "b",
                "c",
                "a"
              ]
            ],
            [
              2,
              "c b c",
              {
                "c": 0.8342565,
                "b": 0.5513765
              },
              [
                "c",
                "b",
                "c"
              ]
            ],
            [
              3,
              "b b a",
              {
                "b": 0.9430448,
                "a": 0.3326656
              },
              [
                "b",
                "b",
                "a"
              ]
            ],
            [
              4,
              "a c c",
              {
                "a": 0.3326656,
                "c": 0.9430448
              },
              [
                "a",
                "c",
                "c"
              ]
            ],
            [
              5,
              "a",
              {
                "a": 1.0
              },
              [
                "a"
              ]
            ]
          ]
        ]
      ]

  * ``TokenDocumentVectorTFIDF`` and ``TokenDocumentVectorBM25`` give a weight against each tokens.

    * ``TokenDocumentVectorTFIDF`` calculate a weight for token by using TF-IDF.

	* Please refer to https://radimrehurek.com/gensim/models/tfidfmodel.html about TF-IDF.

    * ``TokenDocumentVectorBM25`` calculate a weight for token by using Okapi BM25.

	* Please refer to https://en.wikipedia.org/wiki/Okapi_BM25 about Okapi BM25.

* Improved performance when below case.

  * ``(column @ "value") && (column @ "value")``

* [:doc:`/install/ubuntu`] Added support for Ubuntu 20.10 (Groovy Gorilla).

* [:doc:`/install/debian`] Dropped stretch support.

  * It reached EOL.

* [:doc:`/install/centos`] Dropped CentOS 6.

  * It reached EOL.

* [httpd] Updated bundled nginx to 1.19.6.

Fixes
-----

* Fixed a bug that Groonga crash when we use multiple keys drilldown and use multiple accessor as below. [GitHub#1153][Patched by naoa]

  .. code-block::

    table_create Tags TABLE_PAT_KEY ShortText

    table_create Memos TABLE_HASH_KEY ShortText
    column_create Memos tags COLUMN_VECTOR Tags
    column_create Memos year COLUMN_SCALAR Int32

    load --table Memos
    [
    {"_key": "Groonga is fast!", "tags": ["full-text-search"], "year": 2019},
    {"_key": "Mroonga is fast!", "tags": ["mysql", "full-text-search"], "year": 2019},
    {"_key": "Groonga sticker!", "tags": ["full-text-search", "sticker"], "year": 2020},
    {"_key": "Rroonga is fast!", "tags": ["full-text-search", "ruby"], "year": 2020},
    {"_key": "Groonga is good!", "tags": ["full-text-search"], "year": 2020}
    ]

    select Memos \
      --filter '_id > 0' \
      --drilldowns[tags].keys 'tags,year >= 2020' \
      --drilldowns[tags].output_columns _key[0],_key[1],_nsubrecs

    select Memos \
      --filter '_id > 0' \
      --drilldowns[tags].keys 'tags,year >= 2020' \
      --drilldowns[tags].output_columns _key[1],_nsubrecs

* Fixed a bug that the near phrase search did not match when the same phrase occurs multiple times as below.

  .. code-block::

    table_create Entries TABLE_NO_KEY
    column_create Entries content COLUMN_SCALAR Text

    table_create Terms TABLE_PAT_KEY ShortText \
      --default_tokenizer TokenNgram \
      --normalizer NormalizerNFKC121
    column_create Terms entries_content COLUMN_INDEX|WITH_POSITION Entries content

    load --table Entries
    [
    {"content": "a x a x b x x"},
    {"content": "a x x b x"}
    ]

    select Entries \
      --match_columns content \
      --query '*NP2"a b"' \
      --output_columns '_score, content'


Thanks
------

* Hung Nguyen V

* naoa

* timgates42 [Provided the patch at GitHub#1155]

.. _release-10-0-9:

Release 10.0.9 - 2020-12-01
===========================

Improvements
------------

* Improved performance when we specified ``-1`` to ``limit``.

* [:doc:`reference/commands/reference_acquire`] Added a new option ``--auto_release_count``.

  * Groonga reduces a reference count automatically when a request reaching the number of value that is specified in ``--auto_release_count``.

  * For example, the acquired reference of ``Users`` is released automatically after the second ``status`` is processed as below.

    .. code-block::

      reference_acquire --target_name Users --auto_release_count 2
      status # Users is still referred.
      status # Users' reference is released after this command is processed.

  * We can prevent a leak of the release of acquired reference by this option.

* Modify behavior when Groonga evaluated empty ``vector`` and ``uvector``.

  * Empty ``vector`` and ``uvector`` are evaluated to ``false`` in command version 3.

    * This behavior is valid for only command version 3.
    * Note that the different behavior until now.

* [:doc:`/reference/normalizers`] Added a new Normalizer ``NormalizerNFKC130`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 13.0.

* [:doc:`/reference/token_filters`] Added a new TokenFilter ``TokenFilterNFKC130`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 13.0.

* [:doc:`reference/commands/select`] Improved performance for ``"_score = column - X"``.

* [:doc:`reference/commands/reference_acquire`] Improved that ``--reference_acquire`` doesn't get unnecessary reference of index column when we specify the ``--recursive dependent`` option.

  * From this release, the targets of ``--recursive dependent`` are only the target table's key  and the index column that is set to data column.

* [:doc:`reference/commands/select`] Add support for ordered near phrase search.

  * Until now, the near phrase search have only looked for records that the distance of between specified phrases near.
  * This feature look for satisfy the following conditions records.

    * If the distance of between specified phrases is near.
    * If the specified phrases are in line with specified order.

  * This feature use ``*ONP`` as an operator. (Note that the operator isn't ``*NP``.)

  * ``$`` is handled as itself in the query syntax. Note that it isn't a special character in the query syntax.

  * If we use script syntax, this feature use as below.

    .. code-block::

      table_create Entries TABLE_NO_KEY
      column_create Entries content COLUMN_SCALAR Text

      table_create Terms TABLE_PAT_KEY ShortText \
        --default_tokenizer 'TokenNgram("unify_alphabet", false, \
                                        "unify_digit", false)' \
        --normalizer NormalizerNFKC121
      column_create Terms entries_content COLUMN_INDEX|WITH_POSITION Entries content

      load --table Entries
      [
      {"content": "abcXYZdef"},
      {"content": "abebcdXYZdef"},
      {"content": "abcdef"},
      {"content": "defXYZabc"},
      {"content": "XYZabc"},
      {"content": "abc123456789def"},
      {"content": "abc12345678def"},
      {"content": "abc1de2def"}
      ]

      select Entries --filter 'content *ONP "abc def"' --output_columns '_score, content'
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              4
            ],
            [
              [
                "_score",
                "Int32"
              ],
              [
                "content",
                "Text"
              ]
            ],
            [
              1,
              "abcXYZdef"
            ],
            [
              1,
              "abcdef"
            ],
            [
              1,
              "abc12345678def"
            ],
            [
              1,
              "abc1de2def"
            ]
          ]
        ]
      ]

  * If we use query syntax, this feature use as below.

    .. code-block::

       select Entries --query 'content:*ONP "abc def"' --output_columns '_score, content'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               4
             ],
             [
               [
                 "_score",
                 "Int32"
               ],
               [
                 "content",
                 "Text"
               ]
             ],
             [
               1,
               "abcXYZdef"
             ],
             [
               1,
               "abcdef"
             ],
             [
               1,
               "abc12345678def"
             ],
             [
               1,
               "abc1de2def"
             ]
           ]
         ]
       ]

* [httpd] Updated bundled nginx to 1.19.5.

Fixes
-----

* [:doc:`reference/executables/groonga-server-http`] Fixed that Groonga HTTP server finished without waiting all woker threads finished completely.

  * Until now, Groonga HTTP server had started the process of finish after worker threads finished itself process.
    However, worker threads may not finish completely at this timing. Therefore, Groonga HTTP server may crashed according to timing. Because Groonga HTTP server may free area of memory that worker threads are using them yet.

.. _release-10-0-8:

Release 10.0.8 - 2020-10-29
===========================

Improvements
------------

* [:doc:`reference/commands/select`] Added support for large drilldown keys.

  * The maximum on the key size of Groonga's tables are 4KiB.
    However, if we specify multiple keys in drilldown, the size of drilldown keys may be larger than 4KiB.

    * For example, if the total size for ``tag`` key and ``n_like`` key is lager than 4KiB in the following case,
      the drilldown had failed.

    .. code-block::

      select Entries \
        --limit -1 \
        --output_columns tag,n_likes \
        --drilldowns[tag.n_likes].keys tag,n_likes \
        --drilldowns[tag.n_likes].output_columns _value.tag,_value.n_likes,_nsubrecs

    * Because the drilldown packes specifying all keys for drilldown. So, if each the size of drilldown key is large,
      the size of the packed drilldown keys is larger than 4KiB.

  * This feature requires `xxHash <https://github.com/Cyan4973/xxHash>`_ .

    * However, if we install Groonga from package, we can use this feature without doing anything special.
      Because Groonga's package already include `xxHash <https://github.com/Cyan4973/xxHash>`_ .

* [:doc:`reference/commands/select`] Added support for handling as the same dynamic column even if columns refer to different tables.

  * We can't have handled the same dynamic column if columns refer to different tables until now.
    Because the type of columns is different.

  * However, from this version, we can handle the same dynamic column even if columns refer to different tables by casting to built-in types as below.

    .. code-block::

      table_create Store_A TABLE_HASH_KEY ShortText
      table_create Store_B TABLE_HASH_KEY ShortText

      table_create Customers TABLE_HASH_KEY Int32
      column_create Customers customers_A COLUMN_VECTOR Store_A
      column_create Customers customers_B COLUMN_VECTOR Store_B

      load --table Customers
      [
        {"_key": 1604020694, "customers_A": ["A", "B", "C"]},
        {"_key": 1602724694, "customers_B": ["Z", "V", "Y", "T"]},
      ]

      select Customers \
        --filter '_key == 1604020694' \
        --columns[customers].stage output \
        --columns[customers].flags COLUMN_VECTOR \
        --columns[customers].type ShortText \
        --columns[customers].value 'customers_A' \
        --output_columns '_key, customers'

  * We have needed to set ``Store_A`` or ``Store_B`` in the ``type`` of ``customers`` column until now.
  * The type of ``customers_A`` column cast to ``ShortText`` in the above example.
  * By this, we can also set the value of ``customers_B`` in the value of ``customers`` column.
    Because both the key of ``customers_A`` and ``customers_B`` are ``ShortText`` type.

* [:doc:`reference/commands/select`] Improved performance when the number of records for search result are huge.

  * This optimization works when below cases.

    * ``--filter 'column <= "value"'`` or ``--filter 'column >= "value"'``
    * ``--filter 'column == "value"'``
    * ``--filter 'between(...)'`` or ``--filter 'between(_key, ...)'``
    * ``--filter 'sub_filter(reference_column, ...)'``
    * Comparing against ``_key`` such as ``--filter '_key > "value"'``.
    * ``--filter 'geo_in_circle(...)'``

* Updated bundled LZ4 to 1.9.2 from 1.8.2.

* Added support xxHash 0.8

* [httpd] Updated bundled nginx to 1.19.4.

Fixes
-----

* Fixed the following bugs related the browser based administration tool. [GitHub#1139][Reported by sutamin]

  * The problem that Groonga's logo had not been displayed.
  * The problem that the throughput chart had not been displayed on the index page for Japanese.

* [:doc:`/reference/functions/between`] Fixed a bug that ``between(_key, ...)`` is always evaluated by sequential search.

Thanks
------

* sutamin

.. _release-10-0-7:

Release 10.0.7 - 2020-09-29
===========================

Improvements
------------

* [highlight], [:doc:`/reference/functions/highlight_full`] Added support for normalizer options.

* [:doc:`/reference/command/return_code`] Added a new return code ``GRN_CONNECTION_RESET`` for resetting connection.

  * it is returned when an existing connection was forcibly close by the remote host.

* Dropped Ubuntu 19.10 (Eoan Ermine).

  * Because this version has been EOL.

* [httpd] Updated bundled nginx to 1.19.2.

* [:doc:`/reference/executables/grndb`] Added support for detecting duplicate keys.

  * ``grndb check`` is also able to detect duplicate keys since this release.
  * This check valid except a table of ``TABLE_NO_KEY``.
  * If the table that was detected duplicate keys by ``grndb check`` has only index columns, we can recover by ``grndb recover``.

* [:doc:`/reference/commands/table_create`], [:doc:`/reference/commands/column_create`] Added a new option ``--path``

  * We can store specified a table or a column to any path using this option.
  * This option is useful if we want to store a table or a column that
    we often use to fast storage (e.g. SSD) and store them that we don't often
    use to slow storage (e.g. HDD).
  * We can specify both relative path and absolute path in this option.

    * If we specify relative path in this option, the path is resolved the path of ``groonga`` process as the origin.

  * However, if we specify ``--path``, the result of ``dump`` command includes ``--path`` informations.

    * Therefore, if we specify ``--path``, we can't restore to host in different enviroment.

      * Because the directory configuration and the location of ``groonga`` process in different by each enviroment.

    * If we don't want include ``--path`` informations to a dump, we need specify ``--dump_paths no`` in ``dump`` command.

* [:doc:`reference/commands/dump`] Added a new option ``--dump_paths``.

  * ``--dump_paths`` option control whether ``--path`` is dumped or not.
  * The default value of it is ``yes``.
  * If we specify ``--path`` when we create tables or columns and we don't want include ``--path`` informations to a dump, we specify ``no`` into ``--dump_paths`` when we execute ``dump`` command.

* [functions] Added a new function ``string_toknize()``.

  * It tokenizes the column value that is specified in the second argument with the tokenizer that is specified in the first argument.

* [tokenizers] Added a new tokenizer ``TokenDocumentVectorTFIDF`` (experimental).

  * It generates automatically document vector by TF-IDF.

* [tokenizers] Added a new tokenizer ``TokenDocumentVectorBM25`` (experimental).

  * It generates automatically document vector by BM25.

* [:doc:`reference/commands/select`] Added support for near search in same sentence.

Fixes
-----

* [:doc:`/reference/commands/load`] Fixed a bug that ``load`` didn't a return response when we executed it against 257 columns.

  * This bug may occur from 10.0.4 or later.
  * This bug only occur when we load data by using ``[a, b, c, ...]`` format.

    * If we load data by using ``[{...}]``, this bug doesn't occur.

* [MessagePack] Fixed a bug that float32 value wasn't unpacked correctly.

* Fixed the following bugs related multi column index.

  * ``_score`` may be broken with full text search.
  * The records that couldn't hit might hit.
  * Please refer to the following URL for the details about occurrence conditionin of this bug.

    * https://groonga.org/en/blog/2020/09/29/groonga-10.0.7.html#multi-column-index

.. _release-10-0-6:

Release 10.0.6 - 2020-08-29
===========================

Improvements
------------

* [:doc:`reference/commands/logical_range_filter`] Improved search plan for large data.

  * Normally, ``logical_range_filter`` is faster than ``logical_select``.
    However, it had been slower than ``logical_select`` in the below case.

    * If Groonga can't get the number of required records easily, it has the feature that switches index search from sequential search. (Normally, ``logical_range_filter`` uses a sequential search when records of search target are many.)
    * The search process for it is almost the same as ``logical_select`` if the above switching occurred.
      So, ``logical_range_filter`` is severalfold slower than ``logical_select`` in the above case if the search target is large data.
      Because ``logical_range_filter`` executes sort after the search.

  * If we search for large data, Groonga easily use sequential search than until now since this release.
  * Therefore, ``logical_range_filter`` will improve performance. Because the case of the search process almost the same as ``logical_select`` decreases.

* [httpd] Updated bundled nginx to 1.19.1.

* Modify how to install into Debian GNU/Linux.

  * We modify to use ``groonga-apt-source`` instead of ``groonga-archive-keyring``. Because the ``lintian`` command recommends using ``apt-source`` if a package that it puts files under the ``/etc/apt/sources.lists.d/``.

    * The ``lintian`` command is the command which checks for many common packaging errors.
    * Please also refer to the following URL for the details about installation procedures.

      * https://groonga.org/docs/install/debian.html

* [:doc:`reference/commands/logical_select`] Added a support for ``highlight_html`` and ``highlight_full`` .

* Added support for recycling the IDs of records that are deleted when an array without value space delete.[GitHub#mroonga/mroonga#327][Reported by gaeeyo]

  * If an array that doesn't have value space is deleted, deleted IDs are never recycled.
  * Groonga had used large storage space by large ID. Because it uses large storage space by itself.

    * For example, large ID is caused after many adds and deletes like Mroonga's ``mroonga_operations``

* [:doc:`reference/commands/select`] Improved performance of full-text-search without index.

* [:doc:`reference/function`] Improved performance for calling of function that all arguments a variable reference or literal.

* [:doc:`reference/indexing`] Improved performance of offline index construction by using token column. [GitHub#1126][Patched by naoa]

* Improved performance for ``"_score = func(...)"``.

  * The performance when the ``_score`` value calculate by using only function like ``"_score = func(...)"`` improved.

Fixes
-----

* Fixed a bug that garbage may be included in response after response send error.

  * It may occur if a client didn't read all responses and closed the connection.


Thanks
------

* gaeeyo

* naoa

.. _release-10-0-5:

Release 10.0.5 - 2020-07-30
===========================

Improvements
------------

* [:doc:`reference/commands/select`] Added support for storing reference in table that we specify with ``--load_table``.

  * ``--load_table`` is a feature that stores search results to the table in a prepared.

    * If the searches are executed multiple times, we can cache the search results by storing them to this table.
    * We can shorten the search times that the search after the first time by using this table.

  * We can store a reference to other tables into the key of this table as below since this release.

    * We can make a smaller size of this table. Because we only store references without store column values.
    * If we search against this table, we can search by using indexes for reference destination.

    .. code-block::

      table_create Logs TABLE_HASH_KEY ShortText
      column_create Logs timestamp COLUMN_SCALAR Time

      table_create Times TABLE_PAT_KEY Time
      column_create Times logs_timestamp COLUMN_INDEX Logs timestamp

      table_create LoadedLogs TABLE_HASH_KEY Logs

      load --table Logs
      [
      {
        "_key": "2015-02-03:1",
        "timestamp": "2015-02-03 10:49:00"
      },
      {
        "_key": "2015-02-03:2",
        "timestamp": "2015-02-03 12:49:00"
      },
      {
        "_key": "2015-02-04:1",
        "timestamp": "2015-02-04 00:00:00"
      }
      ]

      select \
        Logs \
        --load_table LoadedLogs \
        --load_columns "_key" \
        --load_values "_key" \
        --limit 0

      select \
        --table LoadedLogs \
        --filter 'timestamp >= "2015-02-03 12:49:00"'
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              2
            ],
            [
              [
                "_id",
                "UInt32"
              ],
              [
                "_key",
                "ShortText"
              ],
              [
                "timestamp",
                "Time"
              ]
            ],
            [
              2,
              "2015-02-03:2",
              1422935340.0
            ],
            [
              3,
              "2015-02-04:1",
              1422975600.0
            ]
          ]
        ]
      ]

* [:doc:`reference/commands/select`] Improved sort performance on below cases.

  * When many sort keys need ID resolution.

    * For example, the following expression needs ID resolution.

      * ``--filter true --sort_keys column``

    * For example, the following expression doesn't need ID resolution.
      Because the ``_score`` pseudo column exists in the result table not the source table.

      * ``--filter true --sort_keys _score``

  * When a sort target table has a key.

    * Therefore, ``TABLE_NO_KEY`` isn't supported this improvement.

* [:doc:`reference/commands/select`] Improved performance a bit on below cases.

  * A case of searching for many records matches.
  * A case of drilldown for many records.

* [aggregator] Added support for score accessor for target. [GitHub#1120][Patched by naoa]

  * For example, we can ``_score`` subject to ``aggregator_*`` as below.

    .. code-block::

       table_create Items TABLE_HASH_KEY ShortText
       column_create Items price COLUMN_SCALAR UInt32
       column_create Items tag COLUMN_SCALAR ShortText

       load --table Items
       [
       {"_key": "Book",  "price": 1000, "tag": "A"},
       {"_key": "Note",  "price": 1000, "tag": "B"},
       {"_key": "Box",   "price": 500,  "tag": "B"},
       {"_key": "Pen",   "price": 500,  "tag": "A"},
       {"_key": "Food",  "price": 500,  "tag": "C"},
       {"_key": "Drink", "price": 300,  "tag": "B"}
       ]

       select Items \
         --filter true \
         --drilldowns[tag].keys tag \
         --drilldowns[tag].output_columns _key,_nsubrecs,score_mean \
         --drilldowns[tag].columns[score_mean].stage group \
         --drilldowns[tag].columns[score_mean].type Float \
         --drilldowns[tag].columns[score_mean].flags COLUMN_SCALAR \
         --drilldowns[tag].columns[score_mean].value 'aggregator_mean(_score)'
       [
         [
           0,
           0.0,
           0.0
         ],
         [
           [
             [
               6
             ],
             [
               [
                 "_id",
                 "UInt32"
               ],
               [
                 "_key",
                 "ShortText"
               ],
               [
                 "price",
                 "UInt32"
               ],
               [
                 "tag",
                 "ShortText"
               ]
             ],
             [
               1,
               "Book",
               1000,
               "A"
             ],
             [
               2,
               "Note",
               1000,
               "B"
             ],
             [
               3,
               "Box",
               500,
               "B"
             ],
             [
               4,
               "Pen",
               500,
               "A"
             ],
             [
               5,
               "Food",
               500,
               "C"
             ],
             [
               6,
               "Drink",
               300,
               "B"
             ]
           ],
           {
             "tag": [
               [
                 3
               ],
               [
                 [
                   "_key",
                   "ShortText"
                 ],
                 [
                   "_nsubrecs",
                   "Int32"
                 ],
                 [
                   "score_mean",
                   "Float"
                 ]
               ],
               [
                 "A",
                 2,
                 1.0
               ],
               [
                 "B",
                 3,
                 1.0
               ],
               [
                 "C",
                 1,
                 1.0
               ]
             ]
           }
         ]
       ]

* [:doc:`reference/indexing`] Improved performance of offline index construction on VC++ version.

* [:doc:`reference/commands/select`] Use ``null`` instead ``NaN``, ``Infinity``, and ``-Infinity`` when Groonga outputs result for JSON format.

  * Because JSON doesn't support them.

* [:doc:`reference/commands/select`] Add support fot aggregating standard deviation value.

  * For example, we can calculate a standard deviation for every group as below.

    .. code-block::

        table_create Items TABLE_HASH_KEY ShortText
        column_create Items price COLUMN_SCALAR UInt32
        column_create Items tag COLUMN_SCALAR ShortText

        load --table Items
        [
        {"_key": "Book",  "price": 1000, "tag": "A"},
        {"_key": "Note",  "price": 1000, "tag": "B"},
        {"_key": "Box",   "price": 500,  "tag": "B"},
        {"_key": "Pen",   "price": 500,  "tag": "A"},
        {"_key": "Food",  "price": 500,  "tag": "C"},
        {"_key": "Drink", "price": 300,  "tag": "B"}
        ]

        select Items \
          --drilldowns[tag].keys tag \
          --drilldowns[tag].output_columns _key,_nsubrecs,price_sd \
          --drilldowns[tag].columns[price_sd].stage group \
          --drilldowns[tag].columns[price_sd].type Float \
          --drilldowns[tag].columns[price_sd].flags COLUMN_SCALAR \
          --drilldowns[tag].columns[price_sd].value 'aggregator_sd(price)' \
          --output_pretty yes
        [
          [
            0,
            1594339851.924836,
            0.002813816070556641
          ],
          [
            [
              [
                6
              ],
              [
                [
                  "_id",
                  "UInt32"
                ],
                [
                  "_key",
                  "ShortText"
                ],
                [
                  "price",
                  "UInt32"
                ],
                [
                  "tag",
                  "ShortText"
                ]
              ],
              [
                1,
                "Book",
                1000,
                "A"
              ],
              [
                2,
                "Note",
                1000,
                "B"
              ],
              [
                3,
                "Box",
                500,
                "B"
              ],
              [
                4,
                "Pen",
                500,
                "A"
              ],
              [
                5,
                "Food",
                500,
                "C"
              ],
              [
                6,
                "Drink",
                300,
                "B"
              ]
            ],
            {
              "tag": [
                [
                  3
                ],
                [
                  [
                    "_key",
                    "ShortText"
                  ],
                  [
                    "_nsubrecs",
                    "Int32"
                  ],
                  [
                    "price_sd",
                    "Float"
                  ]
                ],
                [
                  "A",
                  2,
                  250.0
                ],
                [
                  "B",
                  3,
                  294.3920288775949
                ],
                [
                  "C",
                  1,
                  0.0
                ]
              ]
            }
          ]
        ]

    * We can also calculate sample standard deviation to specifing ``aggregate_sd(target, {"unbiased": true})``.

* [Windows] Dropped Visual Studio 2013 support.

Fixes
-----

* [:doc:`reference/executables/groonga-server-http`] Fixed a bug that a request can't halt even if we execute ``shutdown?mode=immediate`` when the response was halted by error occurrence.

* Fixed a crash bug when an error occurs while a request.

  * It only occurs when we use Apache Arrow Format.
  * Groonga crashes when we send request to Groonga again after the previous request was halted by error occurrence.

* [:doc:`/reference/functions/between`] Fixed a crash bug when temporary table is used.

  * For example, if we specify a dynamic column in the first argument for ``between``, Groonga had crashed.

* Fixed a bug that procedure created by plugin is freed unexpectedly.

  * It's only occurred in reference count mode.
  * It's not occurred if we don't use ``plugin_register``.
  * It's not occurred in the process that executes ``plugin_register``.
  * It's occurred in the process that doesn't execute ``plugin_register``.

* Fixed a bug that normalization error occurred while static index construction by ``token_column``. [GitHub#1122][Reported by naoa]

Thanks
------

* naoa

.. _release-10-0-4:

Release 10.0.4 - 2020-06-29
===========================

Improvements
------------

* [:doc:`reference/tables`] Added support for registering 400M records into a hash table.

* [:doc:`reference/commands/select`] Improve scorer performance when the ``_score`` doesn't get recursively values.

  * Groonga get recursively value of ``_score`` when search result is search target.
  * For example, the search targets of ``slices`` are search result. Therefore, if we use ``slice`` in a query, this improvement doesn't ineffective.

* [:doc:`reference/log`] Improved that we output drilldown keys in query-log.

* [:doc:`reference/commands/reference_acquire`], [:doc:`reference/commands/reference_release`] Added new commands for reference count mode.

  * If we need to call multiple ``load`` in a short time, auto close by the reference count mode will degrade performance.
  * We can avoid the performance degrading by calling ``reference_acquire`` before multiple ``load`` and calling ``reference_release`` after multiple ``load``.
    Between ``reference_acquire`` and ``reference_release``, auto close is disabled.

    * Because ``reference_acquire`` acquires a reference of target objects.

  * We can must call ``reference_release`` after you finish performance impact operations.
  * If we don't call ``reference_release``, the reference count mode doesn't work.

* [:doc:`reference/commands/select`] Added support for aggregating multiple groups on one time ``drilldown``.

  * We came to be able to calculate sum or arithmetic mean every different multiple groups on one time ``drilldown`` as below.

    .. code-block::

      table_create Items TABLE_HASH_KEY ShortText
      column_create Items price COLUMN_SCALAR UInt32
      column_create Items quantity COLUMN_SCALAR UInt32
      column_create Items tag COLUMN_SCALAR ShortText

      load --table Items
      [
      {"_key": "Book",  "price": 1000, "quantity": 100, "tag": "A"},
      {"_key": "Note",  "price": 1000, "quantity": 10,  "tag": "B"},
      {"_key": "Box",   "price": 500,  "quantity": 15,  "tag": "B"},
      {"_key": "Pen",   "price": 500,  "quantity": 12,  "tag": "A"},
      {"_key": "Food",  "price": 500,  "quantity": 111, "tag": "C"},
      {"_key": "Drink", "price": 300,  "quantity": 22,  "tag": "B"}
      ]

      select Items \
        --drilldowns[tag].keys tag \
        --drilldowns[tag].output_columns _key,_nsubrecs,price_sum,quantity_sum \
        --drilldowns[tag].columns[price_sum].stage group \
        --drilldowns[tag].columns[price_sum].type UInt32 \
        --drilldowns[tag].columns[price_sum].flags COLUMN_SCALAR \
        --drilldowns[tag].columns[price_sum].value 'aggregator_sum(price)' \
        --drilldowns[tag].columns[quantity_sum].stage group \
        --drilldowns[tag].columns[quantity_sum].type UInt32 \
        --drilldowns[tag].columns[quantity_sum].flags COLUMN_SCALAR \
        --drilldowns[tag].columns[quantity_sum].value 'aggregator_sum(quantity)'
      [
        [
          0,
          0.0,
          0.0
        ],
        [
          [
            [
              6
            ],
            [
              [
                "_id",
                "UInt32"
              ],
              [
                "_key",
                "ShortText"
              ],
              [
                "price",
                "UInt32"
              ],
              [
                "quantity",
                "UInt32"
              ],
              [
                "tag",
                "ShortText"
              ]
            ],
            [
              1,
              "Book",
              1000,
              100,
              "A"
            ],
            [
              2,
              "Note",
              1000,
              10,
              "B"
            ],
            [
              3,
              "Box",
              500,
              15,
              "B"
            ],
            [
              4,
              "Pen",
              500,
              12,
              "A"
            ],
            [
              5,
              "Food",
              500,
              111,
              "C"
            ],
            [
              6,
              "Drink",
              300,
              22,
              "B"
            ]
          ],
          {
            "tag": [
              [
                3
              ],
              [
                [
                  "_key",
                  "ShortText"
                ],
                [
                  "_nsubrecs",
                  "Int32"
                ],
                [
                  "price_sum",
                  "UInt32"
                ],
                [
                  "quantity_sum",
                  "UInt32"
                ]
              ],
              [
                "A",
                2,
                1500,
                112
              ],
              [
                "B",
                3,
                1800,
                47
              ],
              [
                "C",
                1,
                500,
                111
              ]
            ]
          }
        ]
      ]

* [:doc:`reference/executables/groonga`] Added support for ``--pid-path`` in standalone mode.

  * Because ``--pid-path`` had been ignored in standalone mode in before version.

* [:doc:`/reference/commands/io_flush`] Added support for reference count mode.

* [:doc:`reference/commands/logical_range_filter`], [:doc:`/reference/commands/logical_count`] Added support for reference count mode.

* [:doc:`reference/executables/groonga-server-http`] We didn't add header after the last chunk.

  * Because there is a possibility to exist that the HTTP client ignores header after the last chunk.

* [vector_slice] Added support for a vector that has the value of the ``Float32`` type. [GitHub#1112 patched by naoa]

* Added support for parallel offline index construction using token column.

  * We came to be able to construct an offline index on parallel threads from data that are tokenized in advance.

  * We can tune parameters of parallel offline construction by the following environment variables

    * ``GRN_TOKEN_COLUMN_PARALLEL_CHUNK_SIZE`` : We specify how many records are processed per thread.

      * The default value is ``1024`` records.

    * ``GRN_TOKEN_COLUMN_PARALLEL_TABLE_SIZE_THRESHOLD`` : We specify how many source records are required for parallel offline construction.

      * The default value is ``102400`` records.

* [:doc:`reference/commands/select`] Improved performance for ``load_table`` on the reference count mode.

Fixes
-----

* Fixed a bug that the database of Groonga was broken when we search by using the dynamic columns that don't specify a ``--filter`` and stridden over shard.

* Fixed a bug that ``Float32`` type had not displayed on a result of ``schema`` command.

* Fixed a bug that we count in surplus to ``_nsubrecs`` when the reference ``uvector`` hasn't element.

Thanks
------

* naoa

.. _release-10-0-3:

Release 10.0.3 - 2020-05-29
===========================

Improvements
------------

* We came to be able to construct an inverted index from data that are tokenized in advance.

  * The construct of an index is speeded up from this.
  * We need to prepare token column to use this improvement.
  * token column is an auto generated value column like an index column.
  * token column value is generated from source column value by tokenizing the source column value.

  * We can create a token column by setting the source column as below.

    .. code-block::

      table_create Terms TABLE_PAT_KEY ShortText \
        --normalizer NormalizerNFKC121 \
        --default_tokenizer TokenNgram

      table_create Notes TABLE_NO_KEY
      column_create Notes title COLUMN_SCALAR Text

      # The last "title" is the source column.
      column_create Notes title_terms COLUMN_VECTOR Terms title

* [:doc:`reference/commands/select`] We came to be able to specify a ``vector`` for the argument of a function.

  * For example, ``flags`` options of ``query`` can describe by a ``vector`` as below.

    .. code-block::

      select \
        --table Memos \
        --filter 'query("content", "-content:@mroonga", \
                        { \
                          "expander": "QueryExpanderTSV", \
                          "flags": ["ALLOW_LEADING_NOT", "ALLOW_COLUMN"] \
                        })'

* [:doc:`reference/commands/select`] Added a new stage ``result_set`` for dynamic columns.

  * This stage generates a column into a result set table. Therefore, it is not generated if ``query`` or ``filter`` doesn't exist

    * Because if ``query`` or ``filter`` doesn't exist, Groonga doesn't make a result set table.

  * We can't use ``_value`` for the stage. The ``result_set`` stage is for storing value by ``score_column``.

* [vector_slice] Added support for weight vector that has weight of ``Float32`` type. [GitHub#1106 patched by naoa]

* [:doc:`reference/commands/select`] Added support for ``filtered`` stage and ``output`` stage of dynamic columns on drilldowns. [GitHub#1101 patched by naoa][GitHub#1100 patched by naoa]

  * We can use ``filtered`` and ``output`` stage of dynamic columns on drilldowns as with ``drilldowns[Label].stage filtered`` and ``drilldowns[Label].stage output``.

* [:doc:`reference/commands/select`] Added support for ``Float`` type value in aggregating on drilldown.

  * We can aggregate max value, min value, and sum value for ``Float`` type value using ``MAX``, ``MIN``, and ``SUM``.

* [:doc:`reference/functions/query`] [:doc:`reference/functions/geo_in_rectangle`] [:doc:`reference/functions/geo_in_circle`] Added a new option ``score_column`` for ``query()``, ``geo_in_rectangle()``, and ``geo_in_circle()``.

  * We can store a score value by condition using ``score_column``.
  * Normally, Groonga calculate a score by adding scores of all conditions. However, we sometimes want to get a score value by condition.
  * For example, if we want to only use how near central coordinate as score as below, we use ``score_column``.

  .. code-block::

     table_create LandMarks TABLE_NO_KEY
     column_create LandMarks name COLUMN_SCALAR ShortText
     column_create LandMarks category COLUMN_SCALAR ShortText
     column_create LandMarks point COLUMN_SCALAR WGS84GeoPoint

     table_create Points TABLE_PAT_KEY WGS84GeoPoint
     column_create Points land_mark_index COLUMN_INDEX LandMarks point

     load --table LandMarks
     [
       {"name": "Aries"      , "category": "Tower"     , "point": "11x11"},
       {"name": "Taurus"     , "category": "Lighthouse", "point": "9x10" },
       {"name": "Gemini"     , "category": "Lighthouse", "point": "8x8"  },
       {"name": "Cancer"     , "category": "Tower"     , "point": "12x12"},
       {"name": "Leo"        , "category": "Tower"     , "point": "11x13"},
       {"name": "Virgo"      , "category": "Temple"    , "point": "22x10"},
       {"name": "Libra"      , "category": "Tower"     , "point": "14x14"},
       {"name": "Scorpio"    , "category": "Temple"    , "point": "21x9" },
       {"name": "Sagittarius", "category": "Temple"    , "point": "43x12"},
       {"name": "Capricorn"  , "category": "Tower"     , "point": "33x12"},
       {"name": "Aquarius"   , "category": "mountain"  , "point": "55x11"},
       {"name": "Pisces"     , "category": "Tower"     , "point": "9x9"  },
       {"name": "Ophiuchus"  , "category": "mountain"  , "point": "21x21"}
     ]

     select LandMarks \
       --sort_keys 'distance' \
       --columns[distance].stage initial \
       --columns[distance].type Float \
       --columns[distance].flags COLUMN_SCALAR \
       --columns[distance].value 0.0 \
       --output_columns 'name, category, point, distance, _score' \
       --limit -1 \
       --filter 'geo_in_circle(point, "11x11", "11x1", {"score_column": distance}) && category == "Tower"'
     [
       [
         0,
         1590647445.406149,
         0.0002503395080566406
       ],
       [
         [
           [
             5
           ],
           [
             [
               "name",
               "ShortText"
             ],
             [
               "category","ShortText"
             ],
             [
               "point",
               "WGS84GeoPoint"
             ],
             [
               "distance",
               "Float"
             ],
             [
               "_score",
               "Int32"
             ]
           ],
           [
             "Aries",
             "Tower",
             "11x11",
             0.0,
             1
           ],
           [
             "Cancer",
             "Tower",
             "12x12",
             0.0435875803232193,
             1
           ],
           [
             "Leo",
             "Tower",
             "11x13",
             0.06164214760065079,
             1
           ],
           [
             "Pisces",
             "Tower",
             "9x9",
             0.0871751606464386,
             1
           ],
           [
             "Libra",
             "Tower",
             "14x14",
             0.1307627409696579,
             1
           ]
         ]
       ]
     ]

  * The sort by ``_score`` is meaningless in the above example. Because the value of ``_score`` is all ``1`` by ``category == "Tower"``.
    However, we can sort distance from central coordinate using ``score_column``.

* [Windows] Groonga came to be able to output backtrace when it occurs error even if it doesn't crash.

* [Windows] Dropped support for old Windows.

  * Groonga for Windows come to require Windows 8 (Windows Server 2012) or later from 10.0.3.

* [:doc:`reference/commands/select`] Improved sort performance when sort keys were mixed referable sort keys and the other sort keys.

  * We improved sort performance if mixed referable sort keys and the other and there are referable keys two or more.

    * Referable sort keys are sort keys that except below them.

      * Compressed columns
      * ``_value`` against the result of drilldown that is specified multiple values to the key of drilldown.
      * ``_key`` against patricia trie table that has not the key of ``ShortText`` type.
      * ``_score``

  * The more sort keys that except string, a decrease in the usage of memory for sort.

* [:doc:`reference/commands/select`] Improved sort performance when sort keys are all referable keys case.

* [:doc:`reference/commands/select`] Improve scorer performance as a ``_socre = column1*X + column2*Y + ...`` case.

  * This optimization effective when there are many ``+`` or ``*`` in ``_score``.
  * At the moment, it has only effective against ``+`` and ``*``.

* [:doc:`reference/commands/select`] Added support for phrase near search.

  * We can search phrase by phrase by a near search.

    * Query syntax for near phrase search is ``*NP"Phrase1 phrase2 ..."``.
    * Script syntax for near phrase search is ``column *NP "phrase1 phrase2 ..."``.

    * If the search target phrase includes space, we can search for it by surrounding it with ``"`` as below.

      .. code-block::

         table_create Entries TABLE_NO_KEY
         column_create Entries content COLUMN_SCALAR Text

         table_create Terms TABLE_PAT_KEY ShortText \
           --default_tokenizer 'TokenNgram("unify_alphabet", false, \
                                           "unify_digit", false)' \
           --normalizer NormalizerNFKC121
         column_create Terms entries_content COLUMN_INDEX|WITH_POSITION Entries content

         load --table Entries
         [
         {"content": "I started to use Groonga. It's very fast!"},
         {"content": "I also started to use Groonga. It's also very fast! Really fast!"}
         ]

         select Entries --filter 'content *NP "\\"I started\\" \\"use Groonga\\""' --output_columns 'content'
         [
           [
             0,
             1590469700.715882,
             0.03997230529785156
           ],
           [
             [
               [
                 1
               ],
               [
                 [
                   "content",
                   "Text"
                 ]
               ],
               [
                 "I started to use Groonga. It's very fast!"
               ]
             ]
           ]
         ]

* [:doc:`reference/columns/vector`] Added support for ``float32`` weight vector.

  * We can store weight as ``float32`` instead of ``uint32``.
  * We need to add ``WEIGHT_FLOAT32`` flag when execute ``column_create`` to use this feature.

    .. code-block::

       column_create Records tags COLUMN_VECTOR|WITH_WEIGHT|WEIGHT_FLOAT32 Tags

  * However, ``WEIGHT_FLOAT32`` flag isn't available with ``COLUMN_INDEX`` flag for now.

* Added following APIs

  * Added ``grn_obj_is_xxx`` functions. For more information as below.

    * ``grn_obj_is_weight_vector(grn_ctx *ctx, grn_obj *obj)``

      * It returns as a ``bool`` whether the object is a weight vector.

    * ``grn_obj_is_uvector(grn_ctx *ctx, grn_obj *obj)``

      * It returns as a ``bool`` whether the object is a ``uvector``.

        * ``uvector`` is a ``vector`` that size of elements for ``vector`` are fixed.

    * ``grn_obj_is_weight_uvector(grn_ctx *ctx, grn_obj *obj)``

      * It returns as a ``bool`` whether the object is a weight uvector.

  * Added ``grn_type_id_size(grn_ctx *ctx, grn_id id)``.

    * It returns the size of Groonga data type as a ``size_t``.

  * Added ``grn_selector_data_get_xxx`` functions. For more information as below.

    * These functions return selector related data.

      * These functions are supposed to call in selector. If they are called except in selector, they return ``NULL``.

        * ``grn_selector_data_get(grn_ctx *ctx)``

          * It returns all information that relating calling selector as ``grn_selector_data *`` structure.

        * ``grn_selector_data_get_selector(grn_ctx *ctx, grn_selector_data *data)``

          * It returns selector itself as ``grn_obj *``.

        * ``grn_selector_data_get_expr(grn_ctx *ctx, grn_selector_data *data)``

          * It returns selector is used ``--filter`` condition and ``--query`` condition as ``grn_obj *``.

        * ``grn_selector_data_get_table(grn_ctx *ctx, grn_selector_data *data)``

          * It returns target table as ``grn_obj *``

        * ``grn_selector_data_get_index(grn_ctx *ctx, grn_selector_data *data)``

          * It returns index is used by selector as ``grn_obj *``.

        * ``grn_selector_data_get_args(grn_ctx *ctx, grn_selector_data *data, size_t *n_args)``

          * It returns arguments of function that called selector as ``grn_obj **``.

        * ``grn_selector_data_get_result_set(grn_ctx *ctx, grn_selector_data *data)``

          * It returns result table as ``grn_obj *``.

        * ``grn_selector_data_get_op(grn_ctx *ctx, grn_selector_data *data)``

          * It returns how to perform the set operation on existing result set as ``grn_operator``.

  * Added ``grn_plugin_proc_xxx`` functions. For more information as below.

    * ``grn_plugin_proc_get_value_operator(grn_ctx *ctx, grn_obj *value, grn_operator default_operator, const char *context)``

      * It returns the operator of a query as a ``grn_operator``.

        * For example, ``&&`` is returned as a ``GRN_OP_AND``.


    * ``grn_plugin_proc_get_value_bool(grn_ctx *ctx, grn_obj *value, bool default_value, const char *tag)``

      * It returns the value that is specified ``true`` or ``false`` like ``with_transposition`` argument of the below function as a ``bool`` (``bool`` is the data type of C language).

        .. code-block::

           fuzzy_search(column, query, {"max_distance": 1, "prefix_length": 0, "max_expansion": 0, "with_transposition": true})

  * Added ``grn_proc_options_xxx`` functions. For more information as below.

    * ``query()`` only uses them for now.

      * ``grn_proc_options_parsev(grn_ctx *ctx, grn_obj *options, const char *tag, const char *name, va_list args)``

        * This function execute parse options.
        * We had to implement parsing to options ourselves until now, however, we can parse them by just call this function from this version.

      * ``grn_proc_options_parse(grn_ctx *ctx, grn_obj *options, const char *tag, const char *name, ...)``

        * It calls ``grn_proc_options_parsev()``. Therefore, features of this function same ``grn_proc_options_parsev()``.
        * It only differs in the interface compare with ``grn_proc_options_parsev()``.

  * Added ``grn_text_printfv(grn_ctx *ctx, grn_obj *bulk, const char *format, va_list args)``

    * ``grn_text_vprintf`` is deprecated from 10.0.3. We use ``grn_text_printfv`` instead.

  * Added ``grn_type_id_is_float_family(grn_ctx *ctx, grn_id id)``.

    * It returns whether ``grn_type_id`` is ``GRN_DB_FLOAT32`` or ``GRN_DB_FLOAT`` or not as a ``bool``.

  * Added ``grn_dat_cursor_get_max_n_records(grn_ctx *ctx, grn_dat_cursor *c)``.

    * It returns the number of max records the cursor can have as a ``size_t``. (This API is for the DAT table)

  * Added ``grn_table_cursor_get_max_n_records(grn_ctx *ctx, grn_table_cursor *cursor)``.

    * It returns the number of max records the cursor can have as a ``size_t``.
    * It can use against all table type (``TABLE_NO_KEY``, ``TABLE_HASH_KEY``, ``TABLE_DAT_KEY``, and ``TABLE_PAT_KEY``).

  * Added ``grn_result_set_add_xxx`` functions. For more information as below.

    * ``grn_result_set_add_record(grn_ctx *ctx, grn_hash *result_set, grn_posting *posting, grn_operator op)``

      * It adds a record into the table of result sets.
      * ``grn_ii_posting_add_float`` is deprecated from 10.0.3. We use ``grn_rset_add_records()`` instead.

    * ``grn_result_set_add_table(grn_ctx *ctx, grn_hash *result_set, grn_obj *table, double score, grn_operator op)``

      * It adds a table into the result sets.

    * ``grn_result_set_add_table_cursor(grn_ctx *ctx, grn_hash *result_set, grn_table_cursor *cursor, double score, grn_operator op)``

      * It adds records that a table cursor has into the result sets.

  * Added ``grn_vector_copy(grn_ctx *ctx, grn_obj *src, grn_obj *dest)``.

    * It copies a ``vector`` object. It returns whether success copy a ``vector`` object.

  * Added ``grn_obj_have_source(grn_ctx *ctx, grn_obj *obj)``.

    * It returns whether the column has a source column as a ``bool``.

  * Added ``grn_obj_is_token_column(grn_ctx *ctx, grn_obj *obj)``.

    * It returns whether the column is a token column as a ``bool``.

  * Added ``grn_hash_add_table_cursor(grn_ctx *ctx, grn_hash *hash, grn_table_cursor *cursor, double score)``.

    * It's for bulk result set insert. It's faster than inserting records by ``grn_ii_posting_add()``.

Fixes
-----

* Fixed a crash bug if the modules (tokenizers, normalizers, and token filters) are used at the same time from multiple threads.

* Fixed precision of ``Float32`` value when it outputted.

  * The precision of it changes to  8-digit to 7-digit from 10.0.3.

* Fixed a bug that Groonga used the wrong cache when the query that just the parameters of dynamic column different was executed. [GitHub#1102 patched by naoa]

Thanks
------

* naoa

.. _release-10-0-2:

Release 10.0.2 - 2020-04-29
===========================

Improvements
------------

* Added support for ``uvector`` for ``time_classify_*`` functions. [GitHub#1089][Patched by naoa]

  * ``uvector`` is a vector that size of elements for vector are fixed.
  * For example, a vector that has values of Time type as elements is a ``uvector``.

* Improve sort performance if sort key that can't refer value with zero-copy is mixed.

  * Some sort key (e.g. ``_score``) values can't be referred with zero-copy.
  * If there is at least one sort key that can't be referable is included, all sort keys are copied before.
  * With this change, we just copy sort keys that can't be referred. Referable sort keys are just referred without a copy.
  * However, this change may have performance regression when all sort keys are referable.

* Added support for loading weight vector as a JSON string.

  * We can load weight vector as a JSON string as below example.

    .. code-block::

       table_create Tags TABLE_PAT_KEY ShortText
       table_create Data TABLE_NO_KEY
       column_create Data tags COLUMN_VECTOR|WITH_WEIGHT Tags
       column_create Tags data_tags COLUMN_INDEX|WITH_WEIGHT Data tags
       load --table Data
       [
         {"tags": "{\"fruit\": 10, \"apple\": 100}"},
         {"tags": "{\"fruit\": 200}"}
       ]

* Added support for ``Float32`` type.

  * Groonga already has a ``Float`` type. However, it is double precision floating point number. Therefore if we only use single precision floating point number, it is not efficient.
  * We can select more suitable type by adding a ``Float32`` type.

* Added following APIs

  * ``grn_obj_unref(grn_ctx *ctx, grn_obj *obj)``

    * This API is only used on the reference count mode (The reference count mode is a state of ``GRN_ENABLE_REFERENCE_COUNT=yes``.).

      * It calls ``grn_obj_unlink()`` only on the reference count mode. It doesn't do anything except when the reference count mode.
      * We useful it when we need only call ``grn_obj_unlink()`` on the referece count mode.
      * Because as the following example, we don't write condition that whether the reference count mode or not.

        * The example if we don't use ``grn_obj_unref()``.

          .. code-block::

             if (grn_enable_reference_count) {
              grn_obj_unlink(ctx, obj);
             }

        * The example if we use ``grn_obj_unref()``.

          .. code-block::

             grn_obj_ubref(ctx, obj);

  * ``grn_get_version_major(void)``
  * ``grn_get_version_minor(void)``
  * ``grn_get_version_micro(void)``

    * They return Groonga's major, minor, and micor version numbers as a ``uint32_t``.

  * ``grn_posting_get_record_id(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_section_id(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_position(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_tf(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_weight(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_weight_float(grn_ctx *ctx, grn_posting *posting)``
  * ``grn_posting_get_rest(grn_ctx *ctx, grn_posting *posting)``

    * They return information on the posting list.
    * These APIs return value as a ``uint32_t`` except ``grn_posting_get_weight_float``.
    * ``grn_posting_get_weight_float`` returns value as a ``float``.

    * ``grn_posting_get_section_id(grn_ctx *ctx, grn_posting *posting)``

      * Section id is the internal representation of the column name.
      * If column name store in posting list as a string, it is a large amount of information and it use waste capacity.
      * Therefore, Groonga compresses the amount of information and use storage capacity is small by storing column name in the posting list as a number called section id.

    * ``grn_posting_get_tf(grn_ctx *ctx, grn_posting *posting)``

      * ``tf`` of ``grn_posting_get_tf`` is Term Frequency score.

    * ``grn_posting_get_weight_float(grn_ctx *ctx, grn_posting *posting)``

      * It returns weight of token as a ``float``.
      * We suggest using this API when we get a weight of token after this.

        * Because we modify the internal representation of the weight from ``uint32_t`` to ``float`` in the near future.

Fixes
-----

* Fixed a bug that Groonga for 32bit on GNU/Linux may crash.

* Fixed a bug that unrelated column value may be cleared. [GtiHub#1087][Reported by sutamin]

* Fixed a memory leak when we dumped records with ``dump`` command.

* Fixed a memory leak when we specified invalid value into ``output_columns``.

* Fixed a memory leak when we executed ``snippet`` function.

* Fixed a memory leak when we filled the below conditions.

  * If we used dynamic columns on the ``initial`` stage.
  * If we used ``slices`` argument with ``select`` command.

* Fixed a memory leak when we deleted tables with ``logical_table_remove``.

* Fixed a memory leak when we use the reference count mode.

  * The reference count mode is a ``GRN_ENABLE_REFERENCE_COUNT=yes`` state.
  * This mode is experimental. Performance may degrade by this mode.

* Fixed a bug that Groonga too much unlink ``_key`` accessor when we load data for apache arrow format.

Thanks
------

* sutamin

* naoa

.. _release-10-0-1:

Release 10.0.1 - 2020-03-30
===========================

We have been released Groonga 10.0.1.
Because Ubuntu and Windows(VC++ version) package in Groonga 10.0.0 were mistaken.

If we have already used Groonga 10.0.0 for CentOS, Debian, Windows(MinGW version), no problem with continued use it.

Fixes
-----

* Added a missing runtime(vcruntime140_1.dll) in package for Windows VC++ version.

.. _release-10-0-0:

Release 10.0.0 - 2020-03-29
===========================

Improvements
------------

* [httpd] Updated bundled nginx to 1.17.9.

* [httpd] Added support for specifying output type as an extension.

  * For example, we can write ``load.json`` instead of ``load?output_type=json``.

* [:doc:`reference/log`] Outputted a path of opened or closed file into a log of dump level on Linux.

* [:doc:`reference/log`] Outputted a path of closed file into a log of debug level on Windows.  

* Added following API and macros

  * ``grn_timeval_from_double(grn_ctx, double)``

    * This API converts ``double`` type to ``grn_timeval`` type.
    * It returns value of ``grn_timeval`` type.

  * ``GRN_TIMEVAL_TO_NSEC(timeval)``

    * This macro converts value of ``grn_timeval`` type to nanosecond as the value of ``uint64_t`` type.

  * ``GRN_TIME_USEC_TO_SEC(usec)``

    * This macro converts microsecond to second.

* Deprecated the following macro.

  * ``GRN_OBJ_FORMAT_FIN(grn_ctx, grn_obj_format)``

    * We ``grn_obj_format_fin(grn_ctx, grn_obj_format)`` use instead since 10.0.0.

* [:doc:`reference/commands/logical_range_filter`],[:doc:`reference/commands/dump`] Added support for stream output.

  * This feature requires ``command_version 3`` or later. The header content is outputted after the body content.
  * Currently, this feature support only ``dump`` and ``logical_range_filter``.
  * ``logical_range_filter`` always returns the output as a stream on ``command_version 3`` or later.
  * This feature has the following limitations.

    * -1 is only allowed for negative ``limit``
    * MessagePack output isn't supported

  * We a little changed the response contents of JSON by this modify.

    * The key order differs from before versions as below.

      * The key order in before versions.

        .. code-block::

          {
            "header": {...},
            "body": {...}
          }

      * The key order in this version(10.0.0).

        .. code-block::

          {
            "body": {...},
            "header": {...}
          }

  * Disabled caches of ``dump`` and ``logical_range_filter`` when they execute on ``command_version 3``.

    * Because of ``dump`` and ``logical_range_filter`` on ``command_version 3`` returns stream since 10.0.0, Groonga can not cache the whole response.

* [:doc:`reference/commands/logical_range_filter`] Added support for outputting response as Apache Arrow format.

  * Supported data type as below.

    * ``UInt8``
    * ``Int8``
    * ``UInt16``
    * ``Int16``
    * ``UInt32``
    * ``Int32``
    * ``UInt64``
    * ``Int64``
    * ``Time``
    * ``ShortText``
    * ``Text``
    * ``LongText``
    * ``Vector`` of ``Int32``
    * ``Reference vector``

* Supported Ubuntu 20.04 (Focal Fossa).

* Dropped Ubuntu 19.04 (Disco Dingo).

  * Because this version has been EOL.

.. _release-9-1-2:

Release 9.1.2 - 2020-01-29
==========================

Improvements
------------

* [tools] Added a script for copying only files of specify tables or columns.

  * This script name is copy-related-files.rb.
  * This script is useful if we want to extract specifying tables or columns from a huge database.
  * Related files of specific tables or columns may need for reproducing fault.
  * If we difficult to offer a database whole, we can extract related files of target tables or columns by this tool. 

* [:doc:`reference/commands/shutdown`] Accept ``/d/shutdown?mode=immediate`` immediately even when all threads are used.

  * This feature can only use on the Groonga HTTP server.

* Unused objects free immediately by using ``GRN_ENABLE_REFERENCE_COUNT=yes``.

  * This feature is experimental. Performance degrade by this feature.
  * If we load to span many tables, we can expect to keep in the usage of memory by this feature.

* [:doc:`/install/centos`] We prepare ``groonga-release`` by version.

  * Note that the little modification how to install.

* [:doc:`/install/debian`] We use ``groonga-archive-keyring`` for adding the Groonga apt repository.

  * We can easy to add the Groonga apt repository to our system by this improvement.
  * ``groonga-archive-keyring`` includes all information for using the Groonga apt repository. Thus, we need not be conscious of changing of repository information or PGP key by installing this package.
  * ``groonga-archive-keyring`` is deb package. Thus, we can easy to update by ``apt update``.

.. _release-9-1-1:

Release 9.1.1 - 2020-01-07
==========================

Improvements
------------

* [:doc:`/reference/commands/load`] Added support for Apache Arrow format data.

  * If we use Apache Arrow format data, we may reduce parse cost. Therefore, data might be loading faster than other formats.
  * Groonga can also directly input data for Apache Arrow format from other data analysis systems by this improvement.
  * However, Apache Arrow format can use in the HTTP interface only. We can't use it in the command line interface.

* [:doc:`/reference/commands/load`] Added how to load Apache Arrow format data in the document.

* [:doc:`/reference/commands/load`] Improve error message.

  * Response of ``load`` command includes error message also.
  * If we faile data load, Groonga output error detail of ``load`` command by this Improvement.

* [httpd] Updated bundled nginx to 1.17.7.

* [:doc:`reference/executables/groonga-server-http`] Added support for sending command parameters by body of HTTP request.

  * We must set ``application/x-www-form-urlencoded`` to ``Content-Type`` for this case.

* [:doc:`reference/executables/groonga-server-http`] Added how to use HTTP POST in the document.

.. _release-9-1-0:

Release 9.1.0 - 2019-11-29
==========================

Improvements
------------

* Improved the performance of the "&&" operation.

  * For example, the performance of condition expression such as the following is increased.

  * ( A || B ) && ( C || D ) && ( E || F) ...

* [:doc:`/reference/tokenizers/token_mecab`] Added a new option ``use_base_form``

  * We can search using the base form of a token by this option.

  * For example, if we search "支えた" using this option, "支える" is hit also.

Fixes
-----

* Fix a bug that when the accessor is index, performance decreases.

  * For example, it occurs with the query include the following conditions.

    * ``accessor @ query``

    * ``accessor == query``

* Fixed a bug the estimated size of a search result was overflow when the buffer is big enough. [PGroonga#GitHub#115][Reported by Albert Song]

* Improved a test(1) portability. [GitHub#1065][Patched by OBATA Akio]

* Added missing tools.

  * Because ``index-column-diff-all.sh`` and ``object-inspect-all.sh`` had not bundled in before version.

Thanks
------

* Albert Song

* OBATA Akio

.. _release-9-0-9:

Release 9.0.9 - 2019-10-30
==========================

.. note::

    Maybe performance decreases from this version.
    Therefore, If performance decreases than before, please report us with reproducible steps.

Improvements
------------

* [:doc:`reference/log`] Improved that output the sending time of response into query-log.

* [:doc:`reference/commands/status`] Added that the number of current jobs in the ``status`` command response.

* [:doc:`reference/executables/groonga-httpd`] Added support for ``$request_time`` in log.

  * In the previous version, even if we specified the ``$request_time`` in the ``log_format`` directive, it was always 0.00.
  * If we specify the ``$request_time``, groonga-httpd output the correct time form this version.

* [:doc:`reference/executables/groonga-httpd`] Added how to set the ``$request_time`` in the document.

* Supported Ubuntu 19.10 (Eoan Ermine)

* Supported CentOS 8 (experimental)

  * The package for CentOS 8 can't use a part of features(e.g. we can't use ``TokenMecab`` and can't cast to int32 vector from JSON string) for lacking some packages for development.

* [tools] Added a script for executing the ``index_column_diff`` command simply.

  * This script name is index-column-diff-all.sh.
  * This script extracts index columns form Groonga's database and execute the ``index_column_diff`` command to them.

* [tools] Added a script for executing ``object_inspect`` against
  all objects.

  * This script name is object-inspect-all.sh.

Fixes
-----

* Fixed a bug that Groonga crash when we specify the value as the first argument of between.[GitHub#1045][Reported by yagisumi]

Thanks
------

* yagisumi

.. _release-9-0-8:

Release 9.0.8 - 2019-09-27
==========================

Improvements
------------

* [:doc:`reference/commands/log_reopen`] Added a supplementary explanation when we use ``groonga-httpd`` with 2 or more workers.

* Improved that Groonga ignores the index being built.

  * We can get correct search results even if the index is under construction.

  * However, the search is slow because of Groonga out of use the index to search in this case.

* [:doc:`reference/functions/sub_filter`] Added a feature that ``sub_filter`` executes a sequential search when Groonga is building indexes for the target column or the target column hasn't indexed.

  * ``sub_filter`` was an error if the above situation in before
    version.
  * From this version, ``sub_filter`` returns search results if the above situation.
  * However if the above situation, ``sub_filter`` is slow. Because it is executed as a sequential search.

* [:doc:`/install/centos`] Dropped 32-bit package support on CentOS 6.

Fixes
-----

* [:doc:`reference/commands/logical_range_filter`] Fixed a bug that exception about closing the same object twice occurs when we have enough records and the number of records that unmatch filter search criteria is more than the estimated value of it.

.. _release-9-0-7:

Release 9.0.7 - 2019-08-29
==========================

Improvements
------------

* [httpd] Updated bundled nginx to 1.17.3.

  * Contains security fix for CVE-2019-9511, CVE-2019-9513, and CVE-2019-9516.

Fixes
-----

* Fixed a bug that Groonga crash when posting lists were huge.

  * However, this bug almost doesn't occur by general data. Because posting lists don't grow bigger so much by them.

* Fixed a bug that returns an empty result when we specify ``initial`` into a stage of a dynamic column and search for using index. [GitHub#683]

* Fixed a bug that the configure phase didn't detect libedit despite installing it. [GitHub#1030][Patched by yu]

* Fixed a bug that ``--offset`` and ``--limit`` options didn't work
  with ``--sort_keys`` and ``--slices`` options. [clear-code/redmine_full_text_search#70][Reported by a9zawa]

* Fixed a bug that search result is empty when the result of ``select`` command is huge. [groonga-dev,04770][Reported by Yutaro Shimamura]

* Fixed a bug that doesn't use a suitable index when prefix search and suffix search. [GitHub#1007, PGroonga#GitHub#96][Reported by oknj]

Thanks
------

* oknj

* Yutaro Shimamura

* yu

* a9zawa

.. _release-9-0-6:

Release 9.0.6 - 2019-08-05
==========================

Improvements
------------

* Added support for Debian 10 (buster).

Fixes
-----

* [:doc:`reference/commands/select`] Fixed a bug that search is an error when occurring search escalation.

* [:doc:`reference/commands/select`] Fixed a bug that may return wrong search results when we use nested equal condition.

* [geo_distance_location_rectangle] Fixed an example that has wrong ``load`` format. [GitHub#1023][Patched by yagisumi]

* [:doc:`tutorial/micro_blog`] Fixed an example that has wrong search results. [GutHub#1024][Patched by yagisumi]

Thanks
------

* yagisumi

.. _release-9-0-5:

Release 9.0.5 - 2019-07-30
==========================

.. warning::
   There are some critical bugs are found in this release. ``select`` command returns wrong search results.
   We will release the new version (9.0.6) which fixes the issues.
   Please do not use Groonga 9.0.5, and recommends to upgrade to 9.0.6 in the future.
   The detail of this issues are explained at https://groonga.org/en/blog/2019/07/30/groonga-9.0.5.html.

Improvements
------------

* [:doc:`reference/commands/logical_range_filter`] Improved that only apply an optimization when the search target shard is large enough.

  * This feature reduces that duplicate search result between offset when we use same sort key.
  * Large enough threshold is 10000 records by default.

* [:doc:`/reference/normalizers`] Added new option ``unify_to_katakana`` for ``NormalizerNFKC100``.

  * This option normalize hiragana to katakana.
  * For example, ``ゔぁゔぃゔゔぇゔぉ`` is normalized to ``ヴァヴィヴヴェヴォ``.

* [:doc:`reference/commands/select`] Added drilldowns support as a slices parameter.

* [:doc:`reference/commands/select`] Added columns support as a slices parameter.

* [:doc:`reference/commands/select`] Improved that we can refer ``_score`` in the initial stage for slices parameter.

* [:doc:`/reference/functions/highlight_html`], [:doc:`reference/functions/snippet_html`] Improved that extract a keyword also from an expression of before executing a slices when we specify the slices parameter.

* Improved that collect scores also from an expression of before executing a slices when we specify the slices parameter.

* Stopped add 1 in score automatically when add posting to posting list.

  * ``grn_ii_posting_add`` is backward incompatible changed by this change.
    * Caller must increase the score to maintain compatibility.

* Added support for index search for nested equal like ``XXX.YYY.ZZZ == AAA``.

* Reduce rehash interval when we use hash table.

  * This feature improve performance for output result.

* Improved to we can add tag prefix in the query log.

  * We become easy to understand that it is filtered which the condition.

* Added support for Apache Arrow 1.0.0.

  * However, It's not released this version yet.

* Added support for Amazon Linux 2.

Fixes
-----

* Fixed a bug that vector values of JSON like ``"[1, 2, 3]"`` are not indexed.

* Fixed wrong parameter name in ``table_create`` tests. [GitHub#1000][Patch by yagisumi]

* Fixed a bug that drilldown label is empty when a drilldown command is executed by ``command_version=3``. [GitHub#1001][Reported by yagisumi]

* Fixed build error for Windows package on MinGW.

* Fixed install missing COPYING for Windows package on MinGW.

* Fixed a bug that don't highlight when specifing non-text query as highlight target keyword.

* Fixed a bug that broken output of MessagePack format of [:doc:`/reference/commands/object_inspect`]. [GitHub#1009][Reported by yagisumi]

* Fixed a bug that broken output of MessagePack format of ``index_column_diff``. [GitHub#1010][Reported by yagisumi]

* Fixed a bug that broken output of MessagePack format of [:doc:`reference/commands/suggest`]. [GitHub#1011][Reported by yagisumi]

* Fixed a bug that allocate size by realloc isn't enough when a search for a table of patricia trie and so on. [Reported by Shimadzu Corporation]

  * Groonga may be crashed by this bug.

* Fix a bug that ``groonga.repo`` is removed when updating 1.5.0 from ``groonga-release`` version before 1.5.0-1. [groonga-talk:429][Reported by Josep Sanz]

Thanks
------

* yagisumi

* Shimadzu Corporation

* Josep Sanz

.. _release-9-0-4:

Release 9.0.4 - 2019-06-29
==========================

Improvements
------------

* Added support for array literal with multiple elements.

* Added support equivalence operation of a vector.

* [:doc:`reference/commands/logical_range_filter`] Increase outputting logs into query log.

  * ``logical_range_filter`` command comes to output a log for below timing.

    * After filtering by ``logical_range_filter``.
    * After sorting by ``logical_range_filter``.
    * After applying dynamic column.
    * After output results.

  * We can see how much has been finished this command by this feature.

* [:doc:`/reference/tokenizers`] Added document for ``TokenPattern`` description.

* [:doc:`/reference/tokenizers`] Added document for ``TokenTable`` description.

* [:doc:`/reference/tokenizers`] Added document for ``TokenNgram`` description.

* [:doc:`reference/executables/grndb`] Added output operation log into groonga.log

  * ``grndb`` command comes to output execution result and execution process.

* [:doc:`reference/executables/grndb`] Added support for checking empty files.

  * We can check if the empty files exist by this feature.

* [:doc:`reference/executables/grndb`] Added support new option ``--since``

  * We can specify a scope of an inspection.

* [:doc:`reference/executables/grndb`] Added document about new option ``--since``    

* Bundle RapidJSON

  * We can use RapidJson as Groonga's JSON parser partly. (This feature is partly yet)
  * We can more exactly parse JSON by using this.

* Added support for casting to int32 vector from JSON string.

  * This feature requires RapidJSON.

* [:doc:`reference/functions/query`] Added ``default_operator``.

  * We can customize operator when "keyword1 keyword2".
  * "keyword1 Keyword2" is AND operation in default.
  * We can change "keyword1 keyword2"'s operator except AND.

Fixes
-----

* [optimizer] Fix a bug that execution error when specified multiple filter conditions and like ``xxx.yyy=="keyword"``.

* Added missing LICENSE files in Groonga package for Windows(VC++ version).

* Added UCRT runtime into Groonga package for Windows(VC++ version).

* [:doc:`/reference/window_function`] Fix a memory leak.

  * This occurs when multiple windows with sort keys are used. [Patched by Takashi Hashida]

Thanks
------

* Takashi Hashida

.. _release-9-0-3:

Release 9.0.3 - 2019-05-29
==========================

Improvements
------------

* [:doc:`reference/commands/select`] Added more query logs.

  * ``select`` command comes to output a log for below timing.

    * After sorting by drilldown.
    * After filter by drilldown.

  * We can see how much has been finished this command by this feature.

* [:doc:`reference/commands/logical_select`] Added more query logs.

  * ``logical_select`` command comes to output a log for below timing.

    * After making dynamic columns.
    * After grouping by drilldown.
    * After sorting by drilldown.
    * After filter by drilldown.
    * After sorting by ``logical_select``.

  * We can see how much has been finished this command by this feature.

* [:doc:`reference/commands/logical_select`] Improved performance of sort a little when we use ``limit`` option.

* [index_column_diff] Improved performance.

  * We have greatly shortened the execution speed of this command.

* [index_column_diff] Improved ignore invalid reference.

* [index_column_diff] Added support for duplicated vector element case.

* [Normalizers] Added a new Normalizer ``NormalizerNFKC121`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 12.1.

* [TokenFilters] Added a new TokenFilter ``TokenFilterNFKC121`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 12.1.

* [:doc:`reference/executables/grndb`] Added a new option ``--log-flags``

  * We can specify output items of a log as with groonga executable file.
  * See [:doc:`reference/executables/groonga`] to know about supported log flags.

* [:doc:`reference/functions/snippet_html`] Added a new option for changing a return value when no match by search.

* [:doc:`reference/commands/plugin_unregister`] Added support full path of Windows.

* Added support for multiline log message.

  * The multiline log message is easy to read by this feature.

* Output key in Groonga's log when we search by index.

* [:doc:`tutorial/match_columns`] Added a document for indexes with weight.

* [:doc:`reference/commands/logical_range_filter`] Added a explanation for ``order`` parameter.

* [:doc:`reference/commands/object_inspect`] Added an explanation for new statistics ``INDEX_COLUMN_VALUE_STATISTICS_NEXT_PHYSICAL_SEGMENT_ID`` and ``INDEX_COLUMN_VALUE_STATISTICS_N_PHYSICAL_SEGMENTS``.

* Dropped Ubuntu 14.04 support.

Fixes
-----

* [index_column_diff] Fixed a bug that too much ``remains`` are reported.

* Fixed a build error when we use ``--without-onigmo`` option. [GitHub#951] [Reported by Tomohiro KATO]

* Fixed a vulnerability of "CVE: 2019-11675". [Reported by Wolfgang Hotwagner]

* Removed extended path prefix ``\\?\`` at Windows version of Groonga. [GitHub#958] [Reported by yagisumi]

  * This extended prefix causes a bug that plugin can't be found correctly.

Thanks
------

* Tomohiro KATO
* Wolfgang Hotwagner
* yagisumi

.. _release-9-0-2:

Release 9.0.2 - 2019-04-29
==========================

We provide a package for Windows made from VC++ from this release.

We also provide a package for Windows made form MinGW as in the past.
However, we will provide it made from VC++ instead of making from MinGW sooner or later.

Improvements
------------

* [:doc:`/reference/commands/column_create`] Added a new flag ``INDEX_LARGE`` for index column.

  * We can make an index column has space that two times of default by this flag.
  * However, note that it also uses two times of memory usage.
  * This flag useful when index target data are large.
  * Large data must have many records (normally at least 10 millions records) and at least one of the following features.

    * Index targets are multiple columns
    * Index table has tokenizer

* [:doc:`/reference/commands/object_inspect`] Added a new statistics ``next_physical_segment_id`` and ``max_n_physical_segments`` for physical segment information.

  * We can confirm usage of index column space and max value of index column space by this information.

* [:doc:`/reference/commands/logical_select`] Added support for window function over shard.

* [:doc:`/reference/commands/logical_range_filter`] Added support for window function over shard.

* [:doc:`/reference/commands/logical_count`] Added support for window function over shard.

* We provided a package for Windows made from VC++.

* [:doc:`/reference/commands/io_flush`] Added a new option ``--recursive dependent``

  * We can all of the specified flush target object, child objects, corresponding table of an index column and corresponding index column are flush target objects.

Fixes
-----

* Fixed "unknown type name 'bool'" compilation error in some environments.

* Fixed a bug that incorrect output number over Int32 by command of execute via mruby (e.g. ``logical_select``, ``logical_range_filter``, ``logical_count``, etc.). [GitHub#936] [Patch by HashidaTKS]

Thanks
------

* HashidaTKS

.. _release-9-0-1:

Release 9.0.1 - 2019-03-29
==========================

Improvements
------------

* Added support to acccept null for vector value.

  * You can use `select ... --columns[vector].flags COLUMN_VECTOR --columns[vector].value "null"`

* [:doc:`/reference/commands/dump`] Translated document into English.

* Added more checks and logging for invalid indexes. It helps to clarify the index related bugs.

* Improved an explanation about ``GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO`` behavior in news at :ref:`release-8-0-6`.

* [:doc:`/reference/commands/select`] Added new argument ``--load_table``, ``--load_columns`` and ``--load_values``.

  * You can store a result of ``select`` in a table that specifying ``--load_table``.

  * ``--load_values`` option specifies columns of result of ``select``.

  * ``--load_columns`` options specifies columns of table that specifying ``--load_table``.

  * In this way, you can store values of columns that specifying with ``--load_values`` into columns that specifying with ``--load_columns``.

* [:doc:`/reference/commands/select`] Added documentation about ``load_table``, ``load_columns`` and ``load_values``.

* [:doc:`/reference/commands/load`] Added supoort to display a table of load destination in a query log.

  * A name of table of load destination display as string in ``[]`` as below.

  * ``:000000000000000 load(3): [LoadedLogs][3]``

* Added a new API:

  * ``grn_ii_get_flags()``

  * ``grn_index_column_diff()``

  * ``grn_memory_get_usage()``

* Added ``index_column_diff`` command to check broken index column. If you want to log progress of command execution, set log level to debug.

Fixes
-----

* [:doc:`/reference/functions/snippet_html`] Changed to return an empty vector for no match.

  * In such a case, an empty vector ``[]`` is returned instead of ``null``.

* Fixed a warning about possibility of counting threads overflow.
  In real world, it doesn't affect user because enourmous number of threads is not used. [GitHub#904]

* Fixed build error on macOS [GitHub#909] [Reported by shiro615]

* Fixed a stop word handling bug.

  * This bug occurs when we set the first token as a stop word in our query.

  * If this bug occurs, our search query isn't hit.

* [:doc:`/reference/api/global_configurations`] Fixed a typo about parameter name of ``grn_lock_set_timeout``.

* Fixed a bug that deleted records may be matched because of updating indexes incorrectly.

  * It may occure when large number of records is added or deleted.

* Fixed a memory leak when ``logical_range_filter`` returns no records. [GitHub#911] [Patch by HashidaTKS]

* Fixed a bug that query will not match because of loading data is not normalized correctly.
  [PGroonga#GitHub#93, GitHub#912,GitHub#913] [Reported by kamicup and dodaisuke]

  * This bug occurs when load data contains whitespace after KATAKANA and ``unify_kana`` option is used for normalizer.

* Fixed a bug that an indexes is broken during updating indexes.

  * It may occurs when repeating to add large number of records or delete them for a long term.

* Fixed a crash bug that allocated working area is not enough size when updating indexes.

Thanks
------

* shiro615

* HashidaTKS

* kamicup

* dodaisuke

.. _release-9-0-0:

Release 9.0.0 - 2019-02-09
==========================

This is a major version up! But It keeps backward compatibility.
You can upgrade to 9.0.0 without rebuilding database.

Improvements
------------

* [:doc:`/reference/tokenizers`] Added a new tokenizer ``TokenPattern``.

  * You can extract tokens by regular expression.

    * This tokenizer extracts only token that matches the regular expression.

  * You can also specify multiple patterns of regular expression.

* [:doc:`/reference/tokenizers`] Added a new tokenizer ``TokenTable``.

  * You can extract tokens by a value of columns of existing a table.

* [:doc:`/reference/commands/dump`] Added support for dumping binary data.

* [:doc:`/reference/commands/select`] Added support for similer search against index column.

  * If you have used multi column index, you can similar search against all source columns by this feature.

* [:doc:`/reference/normalizers`] Added new option ``remove_blank`` for ``NormalizerNFKC100``.

  * This option remove white spaces.

* [:doc:`/reference/executables/groonga`] Improve display of thread id in log.

  * Because It was easy to confuse thread id and process id on Windows version, it made clear which is a thread id or a process id.

.. _release-8-1-1:

Release 8.1.1 - 2019-01-29
==========================

Improvements
------------

* [:doc:`/reference/commands/logical_select`] Added new argument ``--load_table``, ``--load_columns`` and ``--load_values``.

  * You can store a result of ``logical_select`` in a table that specifying ``--load_table``.

  * ``--load_values`` option specifies columns of result of ``logical_select``.

  * ``--load_columns`` options specifies columns of table that specifying ``--load_table``.

  * In this way, you can store values of columns that specifying with ``--load_values`` into columns that specifying with ``--load_columns``.

* Improve error log when update error of index.

  * Added more information in the log.

    * For example, output source buffer and chunk when occur merge of posting lists error.
    * Also, outputting the log a free space size of a buffer and request size of a buffer when occurs error of allocating a buffer.

* [:doc:`/reference/executables/groonga`] Added a new option ``--log-flags``.

  * We can specify output items of a log of the Groonga.

  * We can output as below items.

    * Timestamp
    * Log message
    * Location(the location where the log was output)
    * Process id
    * Thread id

  * We can specify prefix as below.

    * ``+``

      * This prefix means that "add the flag".

    * ``-``

      * This prefix means that "remove the flag".

    * No prefix means that "replace existing flags".

  * Specifically, we can specify flags as below.

    * ``none``

      * Output nothing into the log.

    * ``time``

      * Output a timestamp into the log.

    * ``message``

      * Output log messages into the log.

    * ``location``

      * Output the location where the log was output( a file name, a line and a function name) and process id.

    * ``process_id``

      * Output a process id into the log.

    * ``pid``

      * This flag is an alias of ``process_id``.

    * ``thread_id``

      * Output thread id into the log.

    * ``all``

      * This flag specifies all flags except ``none`` and ``default`` flags.

    * ``default``

      * Output a timestamp and log messages into the log.

  * We can also specify multiple log flags by separating flags with ``|``.

Fixes
-----

* Fixed a memory leak when occurs index update error.

* [:doc:`/reference/normalizers`] Fixed a bug that stateless normalizers and stateful normalizers return wrong results when we use them at the same time.

    * Stateless normalizers are below.

      * ``unify_kana``
      * ``unify_kana_case``
      * ``unify_kana_voiced_sound_mark``
      * ``unify_hyphen``
      * ``unify_prolonged_sound_mark``
      * ``unify_hyphen_and_prolonged_sound_mark``
      * ``unify_middle_dot``

    * Stateful normalizers are below.

      * ``unify_katakana_v_sounds``
      * ``unify_katakana_bu_sound``
      * ``unify_to_romaji``

.. _release-8-1-0:

Release 8.1.0 - 2018-12-29
==========================

Improvements
------------

* [httpd] Updated bundled nginx to 1.15.8.

Fixes
-----

* Fixed a bug that unlock against DB is always executed after flush when after execute a ``io_flush`` command.

* Fixed a bug that ``reindex`` command doesn't finish when execute a ``reindex`` command against table that has record that has not references.

.. _release-8-0-9:

Release 8.0.9 - 2018-11-29
==========================

Improvements
------------

* [:doc:`/reference/tokenizers`] Improved that output a tokenizer name in error message when create tokenizer fail.

* [:doc:`/reference/tokenizers`][TokenDelimit] Supported that customizing delimiter of a token.

  * You can use token other than whitespace as a token of delimiter.

* [:doc:`/reference/tokenizers`][TokenDelimit] Added new option ``pattern``.

  * You can specify delimiter with regular expression by this option.

* [:doc:`/reference/tokenizers`] Added force_prefix_search value to each token information.

  * "force_prefix" is kept for backward compatibility.

* [:doc:`/reference/token_filters`] Added built-in token filter ``TokenFilterNFKC100``.

  * You can convert katakana to hiragana like NormalizerNFKC100 with a ``unify_kana`` option.

* [:doc:`/reference/token_filters`][TokenFilterStem] Added new option ``algorithm``.

  * You can also stem language other than English(French, Spanish, Portuguese, Italian, Romanian, German, Dutch, Swedish, Norwegian, Danish, Russian, Finnish) by this option.

* [:doc:`/reference/token_filters`][TokenFilterStopWord] Added new option ``column``.

  * You can specify stop word in a column other than is_stop_word column by this option.

* [:doc:`/reference/commands/dump`] Supported output options of token filter options.

  * If you specify a tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options, you can output these options with ``table_list`` command.

* [:doc:`/reference/commands/truncate`] Supported a table that it has token filter option.

  * You can ``truncate`` even a tabel that it has token filter like ``TokenFilterStem`` or ``TokenStopWord`` that has options.

* [:doc:`/reference/commands/schema`] Support output of options of token filter.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that ``unify_to_romaji`` option.

  * You can normalize hiragana and katakana to romaji by this option.

* [query-log][show-condition] Supported "func() > 0" case.

* [Windows] Improved that ensure flushing on unmap.

* Improved error message on opening input file error.

* [httpd] Updated bundled nginx to 1.15.7.

  * contains security fix for CVE-2018-16843 and CVE-2018-16844.

Fixes
-----

* Fixed a memory leak when evaluating window function.

* [:doc:`/reference/executables/groonga-httpd`] Fixed bug that log content may be mixed.

* Fixed a bug that generates invalid JSON when occurs error of slice on output_columns.

* Fixed a memory leak when getting nested reference vector column value.

* Fixed a crash bug when outputting warning logs of index corruption.

* Fix a crash bug when temporary vector is reused in expression evaluation.

  * For example, crash when evaluating an expression that uses a vector as below.

  ``_score = _score + (vector_size(categories) > 0)``

* Fix a bug that hits a value of vector columns deleted by a delete command.[GitHub PGroonga#85][Reported by dodaisuke]

Thanks
------

* dodaisuke

.. _release-8-0-8:

Release 8.0.8 - 2018-10-29
==========================

Improvements
------------

* [:doc:`/reference/commands/table_list`] Supported output options of default tokenizer.

  * If you specify a tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options, you can output these options with ``table_list`` command.

* [:doc:`/reference/commands/select`] Supported normalizer options in sequential match with ``record @ 'query'``.

* [:doc:`/reference/commands/truncate`] Supported a table that it has tokenizer option.

  * You can ``truncate`` even a tabel that it has tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options.

* [:doc:`/reference/tokenizers`][TokenMecab] Added new option ``target_class``

  * This option searches a token of specifying a part-of-speech. For example, you can search only a noun.
  * This option can also specify subclasses and exclude or add specific part-of-speech of specific using ``+`` or ``-``. So, you can also search except a pronoun as below.

    ``'TokenMecab("target_class", "-名詞/代名詞", "target_class", "+")'``

* [:doc:`/reference/commands/io_flush`] Supported locking of a database during a ``io_flush``.

  * Because Groonga had a problem taht is a crash when deleteing a table of a target of a ``io_flush`` during execution of a ``io_flush``.

* [:doc:`/reference/functions/cast_loose`] Added a new function ``cast_loose``.

  * This function cast to a type to specify. If a value to specify can't cast, it become to a default value to specify.

* Added optimize the order of evaluation of a conditional expression.(experimental)

  * You can active this feature by setting environment value as below.

    ``GRN_EXPR_OPTIMIZE=yes``

* Supported ``(?-mix:XXX)`` form for index searchable regular expression. [groonga-dev,04683][Reported by Masatoshi SEKI]

  * ``(?-mix:XXX)`` form treats the same as XXX.

* [httpd] Updated bundled nginx to 1.15.5.

* Supported Ubuntu 18.10 (Cosmic Cuttlefish)

Fixes
-----

* Fixed a bug that the Groonga GQTP server may fail to accept a new connection. [groonga-dev,04688][Reported by Yutaro Shimamura]

  * It's caused when interruption client process without using quit.

Thanks
------

* Masatoshi SEKI
* Yutaro Shimamura

.. _release-8-0-7:

Release 8.0.7 - 2018-09-29
==========================

Improvements
------------

* [:doc:`/reference/tokenizers`][TokenMecab] support outputting metadata of Mecab.

  * Added new option ``include_class`` for ``TokenMecab``.

    This option outputs ``class`` and ``subclass`` in Mecab's metadata.

  * Added new option ``include_reading`` for ``TokenMecab``.

    This option outputs ``reading`` in Mecab's metadata.

  * Added new option ``include_form`` for ``TokenMecab``.

    This option outputs ``inflected_type``, ``inflected_form`` and ``base_form`` in Mecab's metadata.

  * Added new option ``use_reading`` for ``TokenMecab``.

    This option supports a search by kana.

    This option is useful for countermeasure of orthographical variants because it searches with kana.

* [plugin] Groonga now can grab plugins from multiple directories.

  You can specify multiple directories to ``GRN_PLUGINS_PATH`` separated with ":" on non Windows, ";" on Windows.

  ``GRN_PLUGINS_PATH`` has high priority than the existing ``GRN_PLUGINS_DIR``.
  Currently, this option is not supported Windows.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_alphabet`` for ``TokenNgram``.

  If we use ``unify_alphabet`` as ``false``, ``TokenNgram`` uses bigram tokenize method for ASCII character.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_symbol`` for ``TokenNgram``.

  ``TokenNgram("unify_symbol", false)`` is same behavior of ``TokenBigramSplitSymbol``.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_digit`` for ``TokenNgram``.

  If we use ``unify_digit`` as ``false``, If we set false, ``TokenNgram`` uses bigram tokenize method for digits.

* [httpd] Updated bundled nginx to 1.15.4.

Fixes
-----

* Fixed wrong score calculations on some cases.

  * It's caused when adding, multiplication or division numeric to a bool value.
  * It's caused when comparing a scalar and vector columns using ``!=`` or ``==``.

.. _release-8-0-6:

Release 8.0.6 - 2018-08-29
==========================

Improvements
------------

* [:doc:`/reference/tokenizers`][TokenMecab] add ``chunked_tokenize`` and ``chunk_size_threshold`` options.

* [optimizer] support estimation for query family expressions.
  It will generate more effective execution plan with query family expressions such as ``column @ query``, ``column @~ pattern`` and so on.

* [optimizer] plug-in -> built-in
  It's disabled by default for now.
  We can enable it by defining ``GRN_EXPR_OPTIMIZE=yes`` environment variable or using ``expression_rewriters`` table as before.

* Enable sequential search for enough filtered case by default.
  If the current result is enough filtered, sequential search is faster than index search.
  If the current result has only 1% records of all records in a table and less than 1000 records, sequential search is used even when index search is available.

  Cullently, this optimization is applied when search by ``==``, ``>``, ``<``, ``>=``, or ``<=``.

  When a key of a table that has columns specified by the filter is ``ShortText``, you must set ``NormalizerAuto`` to normalizer of the table to apply this optimization.

  You can disable this feature by ``GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO=0.0`` environment variable.

* [load] improve error message.
  Table name is included.

* [load] add ``lock_table`` option.
  If ``--lock_table yes`` is specified, ``load`` locks the target table while updating columns and applying ``--each``.
  This option avoids ``load`` and ``delete`` conflicts but it'll reduce load performance.

* [vector_find] avoid to crash with unsupported modes

Fixes
-----

* [index] fix a bug that offline index construction for text vector with ``HASH_KEY``.
  It creates index with invalid section ID.

* Fix a bug that ``--match_columns 'index[0] || index[9]'`` uses wrong section.

* [highlighter] fix a wrong highlight bug
  It's caused when lexicon is hash table and keyword is less than N of N-gram.

* [mruby] fix a bug that real error is hidden.
  mruby doesn't support error propagation by no argument raise.
  https://github.com/mruby/mruby/issues/290

* [:doc:`/reference/tokenizers`][TokenNgram loose]: fix a not found bug when query has only loose types.
  ``highlight_html()`` with lexicon was also broken.

* Fix a bug that text->number cast ignores trailing garbage.
  "0garbage" should be cast error.

* Fix an optimization bug for ``reference_column >= 'key_value'`` case

.. _release-8-0-5:

Release 8.0.5 - 2018-07-29
==========================

Improvements
------------

* [:doc:`/reference/grn_expr/script_syntax`] Added complementary explain about similar search against Japanese documents.
  [GitHub#858][Patch by Yasuhiro Horimoto]

* [:doc:`/reference/functions/time_classify_day_of_week`] Added a new API: ``time_classify_day_of_week()``.

* Suppressed a warning with ``-fstack-protector``.
  Suggested by OBATA Akio.

* Added a new API: ``time_format_iso8601()``.

* Exported a struct ``grn_raw_string``.

* Added a new API: ``grn_obj_clear_option_values()``.
  It allows you to clear option values on remove (for persistent) / close (for temporary.)

* [log] Reported index column name for error message ``[ii][update][one]``.

* [httpd] Updated bundled nginx to 1.15.2.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 17.10 (Artful Aardvark) support.
  It has reached EOL at July 19, 2018.

* [:doc:`/install/debian`] Dropped jessie support.
  Debian's security and release team will no longer produce updates for jessie.

Fixes
-----

* Fixed returning wrong result after unfinished ``/d/load`` data by POST.

* Fixed wrong function call around KyTea.

* [:doc:`/reference/executables/grndb`] Added a missing label for the ``--force-truncate`` option.

* Fixed crash on closing of a database, when a normalizer provided by a plugin (ex. ``groonga-normalizer-mysql``) is used with any option.

* Fixed a bug that normalizer/tokenizer options may be ignored.
  It's occurred when the same object ID is reused.

.. _release-8-0-4:

Release 8.0.4 - 2018-06-29
==========================

Improvements
------------

* [log] Add sub error for error message ``[ii][update][one]``.

* Added a new API: ``grn_highlighter_clear_keywords()``.

* Added a new predicate: ``grn_obj_is_number_family_bulk()``.

* Added a new API: ``grn_plugin_proc_get_value_mode()``.

* [:doc:`reference/functions/vector_find`] Added a new function ``vector_find()``.

* Suppress memcpy warnings in msgpack.

* Updated mruby from 1.0.0 to 1.4.1.

* [doc][:doc:`/reference/api/grn_obj`] Added API reference for ``grn_obj_is_index_column()``.

* [windows] Suppress printf format warnings.

* [windows] Suppress warning by msgpack.

* [:doc:`/reference/api/grn_obj`][:doc:`/reference/api/plugin`] Added encoding converter.
  rules:

  * grn_ctx::errbuf: grn_encoding

  * grn_logger_put: grn_encoding

  * mruby: UTF-8

  * path: locale

* [mrb] Added ``LocaleOutput``.

* [windows] Supported converting image path to grn_encoding.

* [:doc:`/reference/tokenizers`][TokenMecab] Convert error message encoding.

* [:doc:`/reference/window_functions/window_sum`] Supported dynamic column as a target column.

* [doc][:doc:`/reference/api/grn_obj`] Added API reference for ``grn_obj_is_vector_column()``.

* [:doc:`/reference/commands/column_create`] Added more validations.

  * 1: Full text search index for vector column must have ``WITH_SECTION`` flag.
    (Note that TokenDelmit with ``WITH_POSITION`` without ``WITH_SECTION`` is permitted.
    It's useful pattern for tag search.)

  * 2: Full text search index for vector column must not be multi column index.
    detail: https://github.com/groonga/groonga/commit/08e2456ba35407e3d5172f71a0200fac2a770142

* [:doc:`/reference/executables/grndb`] Disabled log check temporarily.
  Because it's not completed yet.

Fixes
-----

* [:doc:`reference/functions/sub_filter`] Fixed too much score with a too filtered case.

* Fixed build error if KyTea is installed.

* [:doc:`/reference/executables/grndb`] Fixed output channel.

* [query-log][show-condition] Maybe fixed a crash bug.

* [highlighter][lexicon] Fixed a not highlighted bug.
  The keyword wasn't highlighted if keyword length is less than N ("N"-gram.
  In many cases, it's Bigram so "less than 2").

* [windows] Fixed a base path detection bug.
  If system locale DLL path includes 0x5c (``\`` in ASCII) such as "U+8868
  CJK UNIFIED IDEOGRAPH-8868" in CP932, the base path detection is buggy.

* [:doc:`/reference/tokenizers`][TokenNgram] Fixed wrong first character length.
  It's caused for "PARENTHESIZED IDEOGRAPH" characters such as
  "U+3231 PARENTHESIZED IDEOGRAPH STOCK".

.. _release-8-0-3:

Release 8.0.3 - 2018-05-29
==========================

Improvements
------------

* [:doc:`/reference/functions/highlight_html`] Support highlight of results of
  the search by ``NormalizerNFKC100`` or ``TokenNgram``.

* [:doc:`/reference/tokenizers`] Added new option for ``TokenNgram`` that
  ``report_source_location option`` .
  This option used when highlighting with ``highlight_html`` use a lexicon.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_middle_dot option``.
  This option normalizes middle dot. You can search with or without ``・``
  (middle dot) and regardless of ``・`` position.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_katakana_v_sounds option``.
  This option normalizes ``ヴァヴィヴヴェヴォ`` (katakana) to ``バビブベボ`` (katakana).
  For example, you can search ``バイオリン`` (violin) in ``ヴァイオリン`` (violin).

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_katakana_bu_sound option``.
  This option normalizes ``ヴァヴィヴゥヴェヴォ`` (katakana) to ``ブ`` (katakana).
  For example, you can search ``セーブル`` (katakana) and ``セーヴル`` in
  ``セーヴェル`` (katakana).

* [:doc:`reference/functions/sub_filter`] Supported ``sub_filter`` optimization
  for the too filter case.
  this optimize is valid when records are enough narrowed down before
  ``sub_filter`` execution as below.

* [:doc:`/reference/executables/groonga-httpd`] Made all workers context address
  to unique.
  context address is ``#{ID}`` of below query log.

  | #{TIME_STAMP}|#{MESSAGE}
  | #{TIME_STAMP}|#{ID}|>#{QUERY}
  | #{TIME_STAMP}|#{ID}|:#{ELAPSED_TIME} #{PROGRESS}
  | #{TIME_STAMP}|#{ID}|<#{ELAPSED_TIME} #{RETURN_CODE}

* [:doc:`/reference/commands/delete`] Added new options that ``limit``.
  You can limit the number of delete records as below example.
  ``delete --table Users --filter '_key @^ "b"' --limit 4``

* [httpd] Updated bundled nginx to 1.14.0.

Fixes
-----

* [:doc:`/reference/commands/logical_select`] Fixed memory leak when an error occurs
  in filtered dynamic columns.

* [:doc:`/reference/commands/logical_count`] Fixed memory leak on initial dynamic
  column error.

* [:doc:`/reference/commands/logical_range_filter`] Fixed memory leak when an error
  occurs in dynamic column evaluation.

* [:doc:`/reference/tokenizers`] Fixed a bug that the wrong ``source_offset`` when a
  loose tokenizing such as ``loose_symbol`` option.

* [:doc:`/reference/normalizers`] Fixed a bug that FULLWIDTH LATIN CAPITAL LETTERs
  such as ``U+FF21 FULLWIDTH LATIN CAPITAL LETTER A`` aren't normalized to LATIN SMALL
  LETTERs such as ``U+0061 LATIN SMALL LETTER A``.
  If you have been used ``NormalizerNFKC100`` , you must recreate your indexes.

.. _release-8-0-2:

Release 8.0.2 - 2018-04-29
==========================

Improvements
------------

* [:doc:`/reference/executables/grndb`][:ref:`grndb-force-truncate`] Improved
  ``grndb recover --force-truncate`` option that it can be truncated even if
  locks are left on the table.

* [:doc:`/reference/commands/logical_range_filter`] Added ``sort_keys`` option.

* Added a new function ``time_format()``.
  You can specify time format against a column of ``Time`` type.
  You can specify with use format of ``strftime`` .

* [:doc:`/reference/tokenizers`] Support new tokenizer ``TokenNgram``.
  You can change its behavior dynamically via options.
  Here is a list of available options:

    * ``n`` : "N" of Ngram. For example, "3" for trigram.
    * ``loose_symbol`` : Tokenize keywords including symbols, to be searched
      by both queries with/without symbols. For example, a keyword
      "090-1111-2222" will be found by any of "09011112222", "090", "1111",
      "2222" and "090-1111-2222".
    * ``loose_blank`` : Tokenize keywords including blanks, to be searched
      by both queries with/without blanks. For example, a keyword
      "090 1111 2222" will be found by any of "09011112222", "090", "1111",
      "2222" and "090 1111 2222".
    * ``remove_blank`` : Tokenize keywords including blanks, to be searched
      by queries without blanks. For example, a keyword "090 1111 2222" will
      be found by any of "09011112222", "090", "1111" or "2222". Note that
      the keyword won't be found by a query including blanks like
      "090 1111 2222".

* [:doc:`/reference/normalizers`] Support new normalizer "NormalizerNFKC100" based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 10.0.

* [:doc:`/reference/normalizers`] Support options for "NormalizerNFKC51" and "NormalizerNFKC100" normalizers.
  You can change their behavior dynamically.
  Here is a list of available options:

    * ``unify_kana`` : Same pronounced characters in all of full-width
      Hiragana, full-width Katakana and half-width Katakana are regarded as
      the same character.
    * ``unify_kana_case`` : Large and small versions of same letters in all of
      full-width Hiragana, full-width Katakana and half-width Katakana are
      regarded as the same character.
    * ``unify_kana_voiced_sound_mark`` : Letters with/without voiced sound
      mark and semi voiced sound mark in all of full-width Hiragana,
      full-width Katakana and half-width Katakana are regarded as the same
      character.
    * ``unify_hyphen`` : The characters like hyphen are regarded as the hyphen.
    * ``unify_prolonged_sound_mark`` : The characters like prolonged sound mark
      are regarded as the prolonged sound mark.
    * ``unify_hyphen_and_prolonged_sound_mark`` : The characters like hyphen
      and prolonged sound mark are regarded as the hyphen.

* [:doc:`/reference/commands/dump`] Support output of tokenizer's options and
  normalizer's options. Groonga 8.0.1 and earlier versions cannot import dump
  including options for tokenizers or normalizers generated by Groonga 8.0.2
  or later, and it will occurs error due to unsupported information.

* [:doc:`/reference/commands/schema`] Support output of tokenizer's options and
  normalizer's options. Groonga 8.0.1 and earlier versions cannot import schema
  including options for tokenizers or normalizers generated by Groonga 8.0.2
  or later, and it will occurs error due to unsupported information.

* Supported Ubuntu 18.04 (Bionic Beaver)

Fixes
-----

* Fixed a bug that unexpected record is matched with space only query.
  [groonga-dev,04609][Reported by satouyuzh]

* Fixed a bug that wrong scorer may be used.
  It's caused when multiple scorers are used as below.
  ``--match_columns 'title || scorer_tf_at_most(content, 2.0)'``.

* Fixed a bug that it may also take so much time to change "thread_limit".

Thanks
------

* satouyuzh

.. _release-8-0-1:

Release 8.0.1 - 2018-03-29
==========================

Improvements
------------

* [:doc:`/reference/log`] Show ``filter`` conditions in query log.
  It's disabled by default. To enable it, you need to set an environment
  variable ``GRN_QUERY_LOG_SHOW_CONDITION=yes``.

* Install ``*.pdb`` into the directory where ``*.dll`` and ``*.exe``
  are installed.

* [:doc:`/reference/commands/logical_count`] Support ``filtered``
  stage dynamic columns.

* [:doc:`/reference/commands/logical_count`]
  [:ref:`logical-count-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage columns are generated.

* [:doc:`/reference/commands/logical_select`]
  [:ref:`logical-select-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage columns are generated.

* Support LZ4/Zstd/zlib compression for vector data.

* Support alias to accessor such as ``_key``.

* [:doc:`/reference/commands/logical_range_filter`] Optimize
  window function for large result set.
  If we find enough matched records, we don't apply window function
  to the remaining windows.

  TODO: Disable this optimization for small result set if its overhead
  is not negligible. The overhead is not evaluated yet.

* [:doc:`/reference/commands/select`] Added ``match_escalation`` parameter.
  You can force to enable match escalation by ``--match_escalation yes``.
  It's stronger than ``--match_escalation_threshold 99999....999``
  because ``--match_escalation yes`` also works with
  ``SOME_CONDITIONS && column @ 'query'``.
  ``--match_escalation_threshold`` isn't used in this case.

  The default is ``--match_escalation auto``. It doesn't change the
  current behavior.

  You can disable match escalation by ``--match_escalation no``.
  It's the same as ``--match_escalation_threshold -1``.

* [httpd] Updated bundled nginx to 1.13.10.

Fixes
-----

* Fixed memory leak that occurs when a prefix query doesn't match any token.
  [GitHub#820][Patch by Naoya Murakami]

* Fixed a bug that a cache for different databases is used when
  multiple databases are opened in the same process.

* Fixed a bug that a wrong index is constructed.
  This occurs only when the source of a column is a vector column and
  ``WITH_SECTION`` isn't specified.

* Fixed a bug that a constant value can overflow or underflow in
  comparison (>,>=,<,<=,==,!=).

Thanks
------

* Naoya Murakami

.. _release-8-0-0:

Release 8.0.0 - 2018-02-09
==========================

This is a major version up! But It keeps backward compatibility.
You can upgrade to 8.0.0 without rebuilding database.

Improvements
------------

* [:doc:`/reference/commands/select`] Added ``--drilldown_adjuster`` and
  ``--drilldowns[LABEL].adjuster``.
  You can adjust score against result of drilldown.

* [:ref:`online-index-construction`] Changed environment variable name
  ``GRN_II_REDUCE_EXPIRE_ENABLE`` to ``GRN_II_REDUCE_EXPIRE_THRESHOLD``.

  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=0 == GRN_II_REDUCE_EXPIRE_ENABLE=no``.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=-1`` uses
  ``ii->chunk->max_map_seg / 2`` as threshold.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD > 0`` uses
  ``MIN(ii->chunk->max_map_seg / 2, GRN_II_REDUCE_EXPIRE_THRESHOLD)``
  as threshold.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=32`` is the default.

* [:doc:`/reference/functions/between`] Accept ``between()`` without borders.
  If the number of arguments passed to ``between()`` is 3, the 2nd and 3rd
  arguments are handled as the inclusive edges. [GitHub#685]

Fixes
-----

* Fixed a memory leak for normal hash table.
  [GitHub:mroonga/mroonga#190][Reported by fuku1]

* Fix a memory leak for normal array.

* [:doc:`/reference/commands/select`] Stopped to cache when ``output_columns``
  uses not stable function.

* [Windows] Fixed wrong value report on ``WSASend`` error.

Thanks
------

* fuku1

.. _release-7-1-1:

Release 7.1.1 - 2018-01-29
==========================

Improvements
------------

* [:doc:`/install/ubuntu`] Dropped Ubuntu 17.04 (Zesty Zapus) support.
  It has reached EOL at Jan 13, 2018.

* Added quorum match support.
  You can use quorum match in both script syntax and query syntax.
  [groonga-talk,385][Suggested by 付超群]

  TODO: Add documents for quorum match syntax and link to them.

* Added custom similarity threshold support in script syntax.
  You can use custom similarity threshold in script syntax.

  TODO: Add document for the syntax and link to it.

* [:doc:`/reference/executables/grndb`][:ref:`grndb-force-lock-clear`]
  Added ``--force-lock-clear`` option. With this option, ``grndb``
  forces to clear locks of database, tables and data columns. You can
  use your database again even if locks are remained in database,
  tables and data columns.

  But this option very risky. Normally, you should not use it. If your
  database is broken, your database is still broken. This option just
  ignores locks.

* [:doc:`/reference/commands/load`] Added surrogate pairs support in
  escape syntax. For example, ``\uD83C\uDF7A`` is processed as ``🍺``.

* [Windows] Changed to use sparse file on Windows. It reduces disk
  space and there are no performance demerit.

* [:ref:`online-index-construction`] Added
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD`` environment variable to control
  when memory maps are expired in index column. It's ``-1`` by
  default. It means that expire timing is depends on index column
  size. If index column is smaller, expire timing is more. If index
  column is larger, expire timing is less.

  You can use the previous behavior by ``0``. It means that Groonga
  always tries to expire.

* [:doc:`/reference/commands/logical_range_filter`]
  [:ref:`logical-range-filter-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage generated columns are generated.

Fixes
-----

* Reduced resource usage for creating index for reference vector.
  [GitHub#806][Reported by Naoya Murakami]

* [:doc:`/reference/commands/table_create`] Fixed a bug that a table
  is created even when ``token_filters`` is invalid.
  [GitHub#266]

Thanks
------

* 付超群

* Naoya Murakami

.. _release-7-1-0:

Release 7.1.0 - 2017-12-29
==========================

Improvements
------------

* [:doc:`/reference/commands/load`] Improved the ``load``'s
  query-log format.
  Added detail below items in the ``load``'s query-log.

    * outputs number of loaded records.
    * outputs number of error records and columns.
    * outputs number of total records.

* [:doc:`/reference/commands/logical_count`] Improved the
  ``logical_count``'s query-log format.
  Added detail below items in the ``logical_count``'s query-log.

    * outputs number of count.

* [:doc:`/reference/commands/logical_select`] Improve the
  ``logical_select``'s query-log format.
  Added detail below items in the ``logical_select``'s query-log.

    * log N outputs.
    * outputs plain drilldown.
    * outputs labeled drilldown.
    * outputs selected in each shard.
    * use "[...]" for target information.

* [:doc:`/reference/commands/delete`] Improved the ``delete``'s
  query-log format.
  Added detail below items in the ``delete``'s query-log.

    * outputs number of deleted and error records.
    * outputs number of rest number of records.

* [:doc:`/reference/executables/groonga-server-http`] The server
  executed by ``groonga -s`` ensure stopping by C-c.

* Used ``NaN`` and ``Infinity``, ``-Infinity`` instead of Lisp
  representations(``#<nan>`` and  ``#i1/0``, ``#-i1/0``).

* Supported vector for drilldown calc target.

* Partially supported keyword extraction from regexp search.
  It enables ``highlight_html`` and ``snippet_html`` for regexp search.
  [GitHub#787][Reported by takagi01]

* [bulk] Reduced the number of ``realloc()``.
  ``grn_bulk_*()`` API supports it.

  It improves performance for large output case on Windows.
  For example, it causes 100x faster for 100MB over output.

  Because ``realloc()`` is heavy on Windows.

* Enabled ``GRN_II_OVERLAP_TOKEN_SKIP_ENABLE`` only when its value is "yes".

* Deprecated ``GRN_NGRAM_TOKENIZER_REMOVE_BLANK_DISABLE``.
  Use ``GRN_NGRAM_TOKENIZER_REMOVE_BLANK_ENABLE=no`` instead.

* Added new function ``index_column_source_records``.
  It gets source records of index column.[Patch by Naoya Murakami]

* [:doc:`/reference/commands/select`] Supported negative "offset" for "offset + size - limit" >= 0

* Added ``grn_column_cache``.
  It'll improve performance for getter of fixed size column value.

* [:doc:`/reference/executables/groonga`] Added ``--listen-backlog option``.
  You can customize ``listen(2)``'s backlog by this option.

* [httpd] Updated bundled nginx to 1.13.8.

Fixes
-----

* Fixed a memory leak in ``highlight_full``

* Fixed a crash bug by early unlink
  It's not caused by instruction in ``grn_expr_parse()`` but it's caused when
  libgroonga user such as Mroonga uses the following instructions:

    1. ``grn_expr_append_const("_id")``
    2. ``grn_expr_append_op(GRN_OP_GET_VALUE)``

Thanks
------

* takagi01
* Naoya Murakami

.. _release-7-0-9:

Release 7.0.9 - 2017-11-29
==========================

Improvements
------------

* Supported newer version of Apache Arrow. In this release, 0.8.0 or
  later is required for Apache Arrow support.

* [sharding] Added new API for dynamic columns.

  * Groonga::LabeledArguments

* [sharding] Added convenient ``Table#select_all`` method.

* [:doc:`/reference/commands/logical_range_filter`] Supported dynamic
  columns. Note that ``initial`` and ``filtered`` stage are only
  supported.

* [:doc:`/reference/commands/logical_range_filter`] Added documentation
  about ``cache`` parameter and dynamic columns.

* [:doc:`/reference/commands/logical_count`] Supported dynamic
  columns. Note that ``initial`` stage is only supported.

* [:doc:`/reference/commands/logical_count`] Added documentation about
  named parameters.

* [:doc:`/reference/commands/select`] Supported ``--match_columns _key``
  without index.

* [:doc:`/reference/functions/in_values`] Supported to specify more
  than 126 values. [GitHub#760] [GitHub#781] [groonga-dev,04449]
  [Reported by Murata Satoshi]

* [httpd] Updated bundled nginx to 1.13.7.

Fixes
-----

* [httpd] Fixed build error when old Groonga is already installed.
  [GitHub#775] [Reported by myamanishi3]

* [:doc:`/reference/functions/in_values`] Fixed a bug that
  ``in_values`` with too many arguments can cause a crash. This bug is
  found during supporting more than 126 values. [GitHub#780]

* [cmake] Fixed LZ4 and MessagePack detection. [Reported by Sergei
  Golubchik]

* [:ref:`offline-index-construction`] Fixed a bug that offline index
  construction for vector column consumes unnecessary resources. If
  you have a log of elements in one vector column and many records,
  Groonga will crash.
  [groonga-dev,04533][Reported by Toshio Uchiyama]

Thanks
------

* Murata Satoshi
* myamanishi3
* Sergei Golubchik
* Toshio Uchiyama

.. _release-7-0-8:

Release 7.0.8 - 2017-10-29
==========================

Improvements
------------

* [windows] Supported backtrace on crash.
  This feature not only function call history but also source filename
  and number of lines can be displayed as much as possible.
  This feature makes problem solving easier.

* Supported ``( )`` (empty block) only query (``--query "( )"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Supported ``(+)`` (only and block) only query (``--query "(+)"``)
  for ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Supported ``~foo`` (starting with "~") query (``--query "~y"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Modified log level of ``expired`` from ``info`` to ``debug``.
  ``2017-10-29 14:05:34.123456|i| <0000000012345678:0> expired
  i=000000000B123456 max=10 (2/2)`` This message is logged when memory
  mapped area for index is unmapped.  Thus, this log message is useful
  information for debugging, in other words, as it is unnecessary
  information in normal operation, we changed log level from ``info``
  to ``debug``.

* Supported Ubuntu 17.10 (Artful Aardvark)

Fixes
-----

* [dat] Fixed a bug that large file is created unexpectedly in the
  worst case during database expansion process. This bug may occurs
  when you create/delete index columns so frequently. In 7.0.7
  release, a related bug was fixed - "``table_create`` command fails
  when there are many deleted keys", but it turns out that it is not
  enough in the worst case.

* [:doc:`/reference/commands/logical_select`] Fixed a bug that when
  ``offset`` and ``limit`` were applied to multiple shards at the same
  time, there is a case that it returns a fewer number of records
  unexpectedly.

.. _release-7-0-7:

Release 7.0.7 - 2017-09-29
==========================

Improvements
------------

* Supported ``+`` only query (``--query "+"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error.

* [httpd] Updated bundled nginx to 1.13.5.

* [:doc:`/reference/commands/dump`] Added the default argument values
  to the syntax section.

* [:doc:`/reference/command/command_version`] Supported ``--default-command-version 3``.

* Supported caching select result with function call. Now, most of
  existing functions supports this feature. There are two exception,
  when ``now()`` and ``rand()`` are used in query, select result will
  not cached. Because of this default behavior change, new APIs are
  introduced.

  * ``grn_proc_set_is_stable()``
  * ``grn_proc_is_stable()``

  Note that if you add a new function that may return different result
  with the same argument, you must call ``grn_proc_is_stable(ctx,
  proc, GRN_FALSE)``.  If you don't call it, select result with the
  function call is cached and is wrong result for multiple requests.

Fixes
-----

* [windows] Fixed to clean up file handle correctly on failure when
  ``database_unmap`` is executed. There is a case that critical
  section is not initialized when request is canceled before executing
  ``database_unmap``. In such a case, it caused a crach bug.

* [:doc:`/reference/tokenizers`] Fixed document for wrong tokenizer
  names. It should be ``TokenBigramIgnoreBlankSplitSymbolAlpha`` and
  ``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``.

* Changed not to keep created empty file on error.

  In the previous versions, there is a case that empty file keeps
  remain on error.

  Here is the senario to reproduce:

    1. creating new file by grn_fileinfo_open succeeds
    2. mapping file by DO_MAP() is failed

  In such a case, it causes an another error such as
  "already file exists" because of the file which
  isn't under control. so these file should be removed during
  cleanup process.

* Fixed a bug that Groonga may be crashed when search process is
  executed during executing many updates in a short time.

* [:doc:`/reference/commands/table_create`] Fixed a bug that
  ``table_create`` failed when there are many deleted keys.

.. _release-7-0-6:

Release 7.0.6 - 2017-08-29
==========================

Improvements
------------

* Supported prefix match search using multiple
  indexes. (e.g. ``--query "Foo*" --match_columns
  "TITLE_INDEX_COLUMN||BODY_INDEX_COLUMN"``).

* [:doc:`/reference/window_functions/window_count`] Supported
  ``window_count`` function to add count data to result set. It is
  useful to analyze or filter additionally.

* Added the following API

  * ``grn_obj_get_disk_usage():``
  * ``GRN_EXPR_QUERY_NO_SYNTAX_ERROR``
  * ``grn_expr_syntax_expand_query_by_table()``
  * ``grn_table_find_reference_object()``

* [:doc:`/reference/commands/object_inspect`] Supported to show disk
  usage about specified object.

* Supported falling back query parse feature. It is enabled when
  ``QUERY_NO_SYNTAX_ERROR`` flag is set to ``query_flags``. (this
  feature is disabled by default). If this flag is set, query never
  causes syntax error. For example, "A +" is parsed and escaped
  automatically into "A \+". This behavior is useful when application
  uses user input directly and doesn't want to show syntax error to
  user and in log.

* Supported to adjust score for term in query. ">", "<", and "~"
  operators are supported. For example, ">Groonga" increments score of
  "Groonga", "<Groonga" decrements score of "Groonga". "~Groonga"
  decreases score of matched document in the current search
  result. "~" operator doesn't change search result itself.

* Improved performance to remove table. ``thread_limit=1`` is not
  needed for it. The process about checking referenced table existence
  is done without opening objects. As a result, performance is
  improved.

* [httpd] Updated bundled nginx to 1.13.4.

Fixes
-----

* [:doc:`/reference/commands/dump`] Fixed a bug that the 7-th unnamed
  parameter for `--sort_hash_table` option is ignored.

* [:doc:`/reference/commands/schema`] Fixed a typo in command line
  parameter name. It should be `source` instead of `sources`.
  [groonga-dev,04449] [Reported by murata satoshi]

* [:doc:`/reference/commands/ruby_eval`] Fixed crash when ruby_eval
  returned syntax error. [GitHub#751] [Patch by ryo-pinus]

Thanks
------

* murata satoshi

* ryo-pinus

.. _release-7-0-5:

Release 7.0.5 - 2017-07-29
==========================

Improvements
------------

* [httpd] Updated bundled nginx to 1.13.3. Note that this version
  contains security fix for CVE-2017-7529.

* [:doc:`/reference/commands/load`] Supported to load the value of max
  UInt64. In the previous versions, max UInt64 value is converted into
  0 unexpectedlly.

* Added the following API

  * ``grn_window_get_size()`` [GitHub#725] [Patch by Naoya Murakami]

* [:doc:`/reference/functions/math_abs`] Supported ``math_abs()``
  function to calculate absolute value. [GitHub#721]

* Supported to make ``grn_default_logger_set_path()`` and
  ``grn_default_query_logger_set_path()`` thread safe.

* [windows] Updated bundled pcre library to 8.41.

* [:doc:`/reference/commands/normalize`] Improved not to output
  redundant empty string ``""`` on error. [GitHub#730]

* [functions/time] Supported to show error message when division by
  zero was happened. [GitHub#733] [Patch by Naoya Murakami]

* [windows] Changed to map ``ERROR_NO_SYSTEM_RESOURCES`` to
  ``GRN_RESOURCE_TEMPORARILY_UNAVAILABLE``. In the previous versions,
  it returns ``rc=-1`` as a result code. It is not helpful to
  investigate what actually happened. With this fix, it returns
  ``rc=-12``.

* [functions/min][functions/max] Supported vector column. Now you need
  not to care scalar column or vector column to use. [GitHub#735]
  [Patch by Naoya Murakami]

* [:doc:`/reference/commands/dump`] Supported ``--sort_hash_table``
  option to sort by ``_key`` for hash table. Specify
  ``--sort_hash_table yes`` to use it.

* [:doc:`/reference/functions/between`] Supported to specify index
  column. [GitHub#740] [Patch by Naoya Murakami]

* [load] Supported Apache Arrow 0.5.0 or later.

* [:doc:`/troubleshooting/how_to_analyze_error_message`]
  Added howto article to analyze error message in Groonga.

* [:doc:`/install/debian`] Updated required package list to
  build from source.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 16.10 (Yakkety
  Yak) support. It has reached EOL at July 20, 2017.

Fixes
-----

* Fixed to construct correct fulltext indexes against vector column
  which type belongs to text family (```ShortText`` and so on). This
  fix resolves that fulltext search doesn't work well against text
  vector column after updating indexes. [GitHub#494]

* [:doc:`/reference/commands/thread_limit`] Fixed a bug that deadlock
  occurs when thread_limit?max=1 is requested at once.

* [:doc:`/reference/executables/groonga-httpd`] Fixed a mismatch path
  of pid file between default one and restart command assumed. This
  mismatch blocked restarting groonga-httpd. [GitHub#743] [Reported by
  sozaki]

Thanks
------

* Naoya Murakami

.. _release-7-0-4:

Release 7.0.4 - 2017-06-29
==========================

Improvements
------------

* Added physical create/delete operation logs to identify problem for
  troubleshooting. [GitHub#700,#701]

* [:doc:`/reference/functions/in_records`] Improved performance for
  fixed sized column. It may reduce 50% execution time.

* [:doc:`/reference/executables/grndb`] Added ``--log-path`` option.
  [GitHub#702,#703]

* [:doc:`/reference/executables/grndb`] Added ``--log-level`` option.
  [GitHub#706,#708]

* Added the following API

  * ``grn_operator_to_exec_func()``
  * ``grn_obj_is_corrupt()``

* Improved performance for "FIXED_SIZE_COLUMN OP CONSTANT". Supported
  operators are: ``==``, ``!=``, ``<``, ``>``, ``<=`` and ``>=``.

* Improved performance for "COLUMN OP VALUE && COLUMN OP VALUE && ...".

* [:doc:`/reference/executables/grndb`] Supported corrupted object
  detection with ``grndb check``.

* [:doc:`/reference/commands/io_flush`] Supported ``--only_opened``
  option which enables to flush only opened database objects.

* [:doc:`/reference/executables/grndb`] Supported to detect/delete
  orphan "inspect" object. The orphaned "inspect" object is created by
  renamed command name from ``inspect`` to ``object_inspect``.

Fixes
-----

* [rpm][centos] Fixed unexpected macro expansion problem with
  customized build. This bug only affects when rebuilding Groonga SRPM
  with customized ``additional_configure_options`` parameter in spec
  file.

* Fixed missing null check for ``grn_table_setoperation()``. There is a
  possibility of crash bug when indexes are broken. [GitHub#699]

Thanks
------

.. _release-7-0-3:

Release 7.0.3 - 2017-05-29
==========================

Improvements
------------

* [:doc:`/reference/commands/select`] Add document about
  :ref:`full-text-search-with-specific-index-name`.

* [index] Supported to log warning message which record causes posting
  list overflows.

* [:doc:`/reference/commands/load`][:doc:`/reference/commands/dump`]
  Supported Apache Arrow. [GitHub#691]

* [cmake] Supported linking lz4 in embedded static library build.
  [Original patch by Sergei Golubchik]

* [:doc:`/reference/commands/delete`] Supported to cancel.

* [httpd] Updated bundled nginx to 1.13.0

* Exported the following API

  * grn_plugin_proc_get_caller()

* Added index column related function and selector.

  * Added new selector: index_column_df_ratio_between()

  * Added new function: index_column_df_ratio()

Fixes
-----

* [:doc:`/reference/commands/delete`] Fixed a bug that error isn't
  cleared correctly. It affects to following deletions so that it
  causes unexpected behavior.

* [windows] Fixed a bug that IO version is not detected correctly when the
  file is opened with ``O_CREAT`` flag.

* [:doc:`/reference/functions/vector_slice`] Fixed a bug that non 4
  bytes vector columns can't slice. [GitHub#695] [Patch by Naoya
  Murakami]

* Fixed a bug that non 4 bytes fixed vector column can't sequential
  match by specifying index of vector. [GitHub#696] [Patch by Naoya
  Murakami]

* [:doc:`/reference/commands/logical_select`] Fixed a bug that
  "argument out of range" occurs when setting last day of month to the
  min. [GitHub#698]

Thanks
------

* Sergei Golubchik

* Naoya Murakami

.. _release-7-0-2:

Release 7.0.2 - 2017-04-29
==========================

Improvements
------------

* [:doc:`/reference/commands/logical_select`] Supported multiple
  :ref:`logical-select-drilldowns-label-columns-name-window-sort-keys`
  and
  :ref:`logical-select-drilldowns-label-columns-name-window-group-keys`.

* [windows] Updated bundled LZ4 to 1.7.5.

* [cache] Supported persistent cache feature.

* [:doc:`/reference/commands/log_level`] Update English documentation.

* Added the following APIs:

  * ``grn_set_default_cache_base_path()``
  * ``grn_get_default_cache_base_path()``
  * ``grn_persistent_cache_open()``
  * ``grn_cache_default_open()``

* [:option:`groonga --cache-base-path`] Added a new option to use
  persistent cache.

* [:doc:`/reference/executables/groonga-httpd`]
  [:ref:`groonga-httpd-groonga-cache-base-path`] Added new
  configuration to use persistent cache.

* [windows] Updated bundled msgpack to 2.1.1.

* [:doc:`/reference/commands/object_inspect`] Supported not only
  column inspection, but also index column statistics.

* Supported index search for "``.*``" regexp pattern.  This feature is
  enabled by default. Set
  ``GRN_SCAN_INFO_REGEXP_DOT_ASTERISK_ENABLE=no`` environment variable
  to disable this feature.

* [:doc:`/reference/functions/in_records`] Added function to use an
  existing table as condition patterns.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 12.04 (Precise Pangolin)
  support because of EOL.

Fixes
-----

* [:doc:`/reference/commands/logical_select`] Fixed a bug that wrong
  cache is used. This bug was occurred when dynamic column parameter
  is used.

* [:doc:`/reference/commands/logical_select`] Fixed a bug that dynamic
  columns aren't created. It's occurred when no match case.

* [:doc:`/reference/commands/reindex`] Fixed a bug that data is lost
  by reindex. [GitHub#646]

* [httpd] Fixed a bug that response of :doc:`/reference/commands/quit`
  and :doc:`/reference/commands/shutdown` is broken JSON when worker is
  running as another user. [GitHub ranguba/groonga-client#12]

.. _release-7-0-1:

Release 7.0.1 - 2017-03-29
==========================

Improvements
------------

* Exported the following API

  * grn_ii_cursor_next_pos()
  * grn_table_apply_expr()
  * grn_obj_is_data_column()
  * grn_obj_is_expr()
  * grn_obj_is_scalar_column()

* [:doc:`/reference/commands/dump`] Supported to dump weight reference
  vector.

* [:doc:`/reference/commands/load`] Supported to load
  ``array<object>`` style weight vector column. The example of
  ``array<object>`` style is: ``[{"key1": weight1}, {"key2":
  weight2}]``.

* Supported to search ``!(XXX OPERATOR VALUE)`` by index. Supported
  operator is not only ``>`` but also ``>=``, ``<``, ``<=``, ``==``
  and ``!=``.

* Supported index search for "!(column == CONSTANT)". The example in
  this case is: ``!(column == 29)`` and so on.

* Supported more "!" optimization in the following patterns.

  * ``!(column @ "X") && (column @ "Y")``
  * ``(column @ "Y") && !(column @ "X")``
  * ``(column @ "Y") &! !(column @ "X")``

* Supported to search ``XXX || !(column @ "xxx")`` by index.

* [:doc:`/reference/commands/dump`] Changed to use ``'{"x": 1, "y":
  2}'`` style for not referenced weight vector. This change doesn't
  affect to old Groonga because it already supports one.

* [experimental] Supported ``GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE``
  environment variable. This variable controls whether query
  optimization which is based on estimated size is applied or not.
  This feature is disabled by default. Set
  ``GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE=yes`` if you want to try it.

* [:doc:`/reference/commands/select`] Added query log for ``columns``,
  ``drilldown`` evaluation.

* [:doc:`/reference/commands/select`] Changed query log format for
  ``drilldown``. This is backward incompatible change, but it only
  affects users who convert query log by own programs.

* [:doc:`/reference/commands/table_remove`] Reduced temporary memory
  usage. It's enabled when the number of max threads is 0.

* [:doc:`/reference/commands/select`] ``columns[LABEL](N)`` is used
  for query log format instead of ``columns(N)[LABEL]``..

* [:doc:`/tutorial/query_expansion`] Updated example to use vector
  column because it is recommended way. [Reported by Gurunavi, Inc]

* Supported to detect canceled request while locking. It fixes the
  problem that ``request_cancel`` is ignored unexpectedly while locking.

* [:doc:`/reference/commands/logical_select`] Supported ``initial``
  and ``filtered`` stage dynamic columns. The examples are:
  ``--columns[LABEL].stage initial`` or ``--columns[LABEL].stage
  filtered``.

* [:doc:`/reference/commands/logical_select`] Supported
  ``match_columns``, ``query`` and ``drilldown_filter`` option.

* [:doc:`/reference/functions/highlight_html`] Supported similar
  search.

* [:doc:`/reference/commands/logical_select`] Supported ``initial``
  and stage dynamic columns in labeled drilldown. The example is:
  ``--drilldowns[LABEL].stage initial``.

* [:doc:`/reference/commands/logical_select`] Supported window
  function in dynamic column.

* [:doc:`/reference/commands/select`] Added documentation about
  dynamic columns.

* [:doc:`/reference/window_function`] Added section about window
  functions.

* [:doc:`/install/centos`] Dropped CentOS 5 support because of EOL.

* [httpd] Updated bundled nginx to 1.11.12

* Supported to disable AND match optimization by environment variable.
  You can disable this feature by
  ``GRN_TABLE_SELECT_AND_MIN_SKIP_ENABLE=no``. This feature is enable
  by default.

* [:doc:`/reference/functions/vector_new`] Added a new function to
  create a new vector.

* [:doc:`/reference/commands/select`] Added documentation about
  ``drilldown_filter``.

Fixes
-----

* [:doc:`/reference/commands/lock_clear`] Fixed a crash bug against
  temporary database.

* Fixed a problem that dynamically updated index size was increased
  for natural language since Grooonga 6.1.4.

* [:doc:`/reference/commands/select`] Fixed a bug that "A && B.C @ X"
  may not return records that should be matched.

* Fixed a conflict with ``grn_io_flush()`` and
  ``grn_io_expire()``. Without this change, if ``io_flush`` and ``load``
  command are executed simultaneously in specific timing, it causes a
  crash bug by access violation.

* [:doc:`/reference/commands/logical_table_remove`] Fixed a crash bug
  when the max number of threads is 1.

Thanks
------

* Gurunavi, Inc.

.. _release-7-0-0:

Release 7.0.0 - 2017-02-09
==========================

Improvements
------------

* [:doc:`/reference/functions/in_values`] Supported sequential search
  for reference vector column. [Patch by Naoya Murakami] [GitHub#629]

* [:doc:`/reference/commands/select`] Changed to report error instead
  of ignoring on invalid ``drilldown[LABEL].sort_keys``.

* [:doc:`/reference/commands/select`] Removed needless metadata
  updates on DB. It reduces the case that database lock remains
  even though ``select`` command is executed. [Reported by aomi-n]

* [:doc:`/reference/commands/lock_clear`] Changed to clear metadata lock
  by lock_clear against DB.

* [:doc:`/install/centos`] Enabled EPEL by default to install Groonga
  on Amazon Linux.

* [:doc:`/reference/functions/query`] Supported "@X" style in script
  syntax for prefix("@^"), suffix("@$"), regexp("@^") search.

* [:doc:`/reference/functions/query`] Added documentation about
  available list of mode. The default mode is ``MATCH`` ("@") mode
  which executes full text search.

* [rpm][centos] Supported groonga-token-filter-stem package which
  provides stemming feature by ``TokenFilterStem`` token filter on
  CentOS 7. [GitHub#633] [Reported by Tim Bellefleur]

* [:doc:`/reference/window_functions/window_record_number`] Marked
  ``record_number`` as deprecated. Use ``window_record_number``
  instead. ``record_number`` is still available for backward
  compatibility.

* [:doc:`/reference/window_functions/window_sum`] Added ``window_sum``
  window function. It's similar behavior to window function sum() on
  PostgreSQL.

* Supported to construct offline indexing with in-memory (temporary)
  ``TABLE_DAT_KEY`` table. [GitHub#623] [Reported by Naoya Murakami]

* [onigmo] Updated bundled Onigmo to 6.1.1.

* Supported ``columns[LABEL].window.group_keys``. It's used to apply
  window function for every group.

* [:doc:`/reference/commands/load`] Supported to report error on
  invalid key. It enables you to detect mismatch type of key.

* [:doc:`/reference/commands/load`] Supported ``--output_errors yes``
  option. If you specify "yes", you can get errors for each load
  failed record. Note that this feature requires command version 3.

* [:doc:`/reference/commands/load`] Improve error message on table key
  cast failure. Instead of "cast failed", type of table key and target
  type of table key are also contained in error message.

* [httpd] Updated bundled nginx to 1.11.9.

Fixes
-----

* Fixed a bug that nonexistent sort keys for ``drilldowns[LABEL]`` or
  ``slices[LABEL]`` causes invalid JSON parse error. [Patch by Naoya
  Murakami] [GitHub#627]

* Fixed a bug that access to nonexistent sub records for group causes
  a crash.  For example, This bug affects the case when you use
  ``drilldowns[LABEL].sort_keys _sum`` without specifying
  ``calc_types``.  [Patch by Naoya Murakami] [GitHub#625]

* Fixed a crash bug when tokenizer has an error. It's caused when
  tokenizer and token filter are registered and tokenizer has an
  error.

* [:doc:`/reference/window_functions/window_record_number`] Fixed a
  bug that arguments for window function is not correctly
  passed. [GitHub#634][Patch by Naoya Murakami]

Thanks
------

* Naoya Murakami
* aomi-n

The old releases
================

.. toctree::
   :maxdepth: 2

   news/6.x
   news/5.x
   news/4.x
   news/3.x
   news/2.x
   news/1.3.x
   news/1.2.x
   news/1.1.x
   news/1.0.x
   news/0.x
   news/senna
