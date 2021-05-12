.. -*- rst -*-

.. groonga-command
.. database: functions_sub_filter

``sub_filter``
==============

Summary
-------

``sub_filter`` evaluates ``filter_string`` in ``scope`` context.

``sub_filter`` can be used in only ``--filter`` in
:doc:`/reference/commands/select`.

Syntax
------

``sub_filter`` requires two arguments. They are ``scope`` and
``filter_string``.

::

  sub_filter(scope, filter_string)


Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/sub_filter/usage_setup_schema.log
.. table_create Comment TABLE_PAT_KEY UInt32
.. column_create Comment name COLUMN_SCALAR ShortText
.. column_create Comment content COLUMN_SCALAR ShortText
.. table_create Blog TABLE_PAT_KEY ShortText
.. column_create Blog title COLUMN_SCALAR ShortText
.. column_create Blog content COLUMN_SCALAR ShortText
.. column_create Blog comments COLUMN_VECTOR Comment
.. column_create Comment blog_comment_index COLUMN_INDEX Blog comments
.. table_create Lexicon TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram
.. column_create Lexicon comment_content COLUMN_INDEX|WITH_POSITION Comment content
.. column_create Lexicon comment_name COLUMN_INDEX|WITH_POSITION Comment name
.. column_create Lexicon blog_content COLUMN_INDEX|WITH_POSITION Blog content

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/sub_filter/usage_setup_data.log
.. load --table Comment
.. [
.. {"_key": 1, "name": "A", "content": "groonga"},
.. {"_key": 2, "name": "B", "content": "groonga"},
.. {"_key": 3, "name": "C", "content": "rroonga"},
.. {"_key": 4, "name": "A", "content": "mroonga"},
.. ]
.. load --table Blog
.. [
.. {"_key": "groonga's blog", "content": "content of groonga's blog", comments: [1, 2, 3]},
.. {"_key": "mroonga's blog", "content": "content of mroonga's blog", comments: [2, 3, 4]},
.. {"_key": "rroonga's blog", "content": "content of rroonga's blog", comments: [3]},
.. ]

Here is the simple usage of ``sub_filter`` function which extracts the
blog entry commented by user 'A'.

.. groonga-command
.. include:: ../../example/reference/functions/sub_filter/usage_without_sub_filter.log
.. select Blog --output_columns _key --filter "comments.name @ \"A\" && comments.content @ \"groonga\""

When executing the above query, not only "groonga's blog", but also "mroonga's blog".
This is not what you want because user "A" does not mention "groonga" to "mroonga's blog".

Without sub_filter, it means that following conditions are met.

* There is at least one record that user "A" commented out.
* There is at least one record that mentioned about "groonga".

.. groonga-command
.. include:: ../../example/reference/functions/sub_filter/usage_with_sub_filter.log
.. select Blog --output_columns _key --filter 'sub_filter(comments, "name @ \\"A\\" && content @ \\"groonga\\"")'

On the other hand, executing the above query returns the intended result.
Because the arguments of sub_filter is evaluated in comments column's context.

It means that sub_filter requires the following condition is met.

* There are the records that user "A" mentions about "groonga".


Parameters
----------

There are two required parameter, ``scope`` and ``filter_string``.

``scope``
^^^^^^^^^

Specifies a column of the table that is specified by ``table``
parameter in ``select``. The column has a limitation. The limitation
is described later. ``filter_string`` is evaluated in the column
context. It means that ``filter_string`` is evaluated like
``select --table TYPE_OF_THE_COLUMN --filter FILTER_STRING``.

The specified column type must be a table. In other words, the column
type must be reference type.

You can chain columns by ``COLUMN_1.COLUMN_2.COLUMN_3...COLUMN_N``
syntax. For example, ``user.group.name``.

See :ref:`select-table` about ``table`` parameter in ``select``.

``filter_string``
^^^^^^^^^^^^^^^^^

Specifies a search condition in
:doc:`/reference/grn_expr/script_syntax`. It is evaluated in ``scope``
context.

Return value
------------

``sub_filter`` returns whether any record is matched or not. If one or
more records are matched, it returns ``true``. Otherwise, it returns
``false``.

See also
--------

* :doc:`/reference/commands/select`
* :doc:`/reference/grn_expr/script_syntax`
