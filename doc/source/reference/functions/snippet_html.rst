.. -*- rst -*-

.. groonga-command
.. database: functions_snippet_html

``snippet_html``
================

Summary
-------

This function extracts snippets of target text around search keywords
(``KWIC``. ``KeyWord In Context``).

The snippets are prepared for embedding HTML. Special characters such
as ``<`` and ``>`` are escapsed as ``&lt;`` and ``&gt;``. Keyword is
surrounded with ``<span class="keyword">`` and ``</span>``. For
example, a snippet of ``I am a groonga user. <3`` for keyword
``groonga`` is ``I am a <span class="keyword">groonga</span>
user. &lt;3``.

This function is a HTML specific version of :doc:`snippet`. Generally,
this function is enough for normal Web application use.

Syntax
------

``snippet_html`` has only one parameter::

  snippet_html(column)

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/snippet_html/usage_setup.log
.. table_create Documents TABLE_NO_KEY
.. column_create Documents content COLUMN_SCALAR Text
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram  --normalizer NormalizerAuto
.. column_create Terms documents_content_index COLUMN_INDEX|WITH_POSITION Documents content
.. load --table Documents
.. [
.. ["content"],
.. ["Groonga is a fast and accurate full text search engine based on inverted index. One of the characteristics of groonga is that a newly registered document instantly appears in search results. Also, groonga allows updates without read locks. These characteristics result in superior performance on real-time applications."],
.. ["Groonga is also a column-oriented database management system (DBMS). Compared with well-known row-oriented systems, such as MySQL and PostgreSQL, column-oriented systems are more suited for aggregate queries. Due to this advantage, groonga can cover weakness of row-oriented systems."]
.. ]

``snippet_html`` can be used in only ``--output_columns`` in
:doc:`/reference/commands/select`.

You also need to specify ``--query`` and/or ``--filter``. Keywords are
extracted from ``--query`` and ``--filter`` arguments.

The following example uses ``--query "fast performance"``. In this
case, ``fast`` and ``performance`` are used as keywords.

.. groonga-command
.. include:: ../../example/reference/functions/snippet_html/usage_basic.log
.. select Documents --output_columns "snippet_html(content)" --command_version 2 --match_columns content --query "fast performance"

``--query "fast performance"`` matches to only the first record's
content. ``snippet_html(content)`` extracts two text parts that
include the keywords ``fast`` or ``performance`` and surrounds the
keywords with ``<span class="keyword">`` and ``</span>``.

The max number of text parts is 3. If there are 4 or more text parts
that include the keywords, only the leading 3 parts are only used.

The max size of a text part is 200byte. The unit is bytes not
characters. The size doesn't include inserted ``<span class="keyword">``
and ``</span>``.

Both the max number of text parts and the max size of a text part
aren't customizable.

If you want to customize them, you need to use :doc:`snippet` instead
of this function.

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/snippet_html/usage_string_literal.log
.. select Documents --output_columns 'snippet_html("Groonga is very fast fulltext search engine.")' --command_version 2 --match_columns content --query "fast performance"

Return value
------------

This function returns an array of string or ``null``. If This function
can't find any snippets, it returns ``null``.

An element of array is a snippet::

  [SNIPPET1, SNIPPET2, SNIPPET3]

A snippet includes one or more keywords. The max byte size of a
snippet except open tag and close tag is 200byte. The unit isn't the
number of characters.

Open tag is ``<span class="keyword">`` and close tag is ``</span>``.

The array size is larger than or equal to 1 and less than or equal
to 3.

If you want to customize the max byte size of snippet and/or the max
number of snippets, use :doc:`snippet` instead.

See also
--------

* :doc:`snippet`
* :doc:`/reference/commands/select`
