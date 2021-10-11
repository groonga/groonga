.. -*- rst -*-

.. groonga-command
.. database: functions_snippet

``snippet``
===========

Summary
-------

This function extracts snippets of target text around search
keywords (``KWIC``. ``KeyWord In Context``).

If you want to use this function for normal Web application,
:doc:`snippet_html` may be suitable. It's a HTML specific version of
this function.

Syntax
------

``snippet`` requires at least one parameter that is the snippet target
text::

  snippet(column, ...)

You can specify one ore more tuples of keyword, open tag and close tag::

  snippet(column,
          "keyword1", "open-tag1", "close-tag1",
          "keyword2", "open-tag2", "close-tag2",
          ...)

If you specify default open tag and default close tag, you can specify
only keywords::

  snippet(column,
          "keyword1",
          "keyword2",
          ...,
          {
            "default_open_tag": "open-tag",
            "default_close_tag": "close-tag"
          })

.. versionadded:: 11.0.9

   If you specify default open tag and default close tag and omit
   keywords, keywords are extracted from the current condition
   automatically like :doc:`snippet_html`::

     snippet(column,
             {
               "default_open_tag": "open-tag",
               "default_close_tag": "close-tag"
             })

You can specify options as the last argument with all syntaxes::

  snippet(column,
          ...,
          {
            "width": 200,
            "max_n_results": 3,
            "skip_leading_spaces": true,
            "html_escape": false,
            "prefix": null,
            "suffix": null,
            "normalizer": null,
            "default_open_tag": null,
            "default_close_tag": null,
            "default": null,
            "delimiter_pattern": null,
          })

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_setup.log
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

``snippet`` extracts keywords from conditions specified in ``--query``
and/or ``--filter`` automatically when you specify
``default_open_tag`` option and ``default_close_tag`` and don't
specify keywords. It's similar to :doc:`snippet_html`.

The following example uses ``--query "fast performance"``. In this
case, ``fast`` and ``performance`` are used as keywords.

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_keywords_from_conditions.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             { \
..                                "default_open_tag": "[", \
..                                "default_close_tag": "]" \
..                             })' \
..   --match_columns content \
..   --query "fast performance"

``--query "fast performance"`` matches to only the first record's
content. This ``snippet`` extracts two text parts that include the
keywords ``fast`` or ``performance`` and surrounds the keywords with
``[`` and ``]``.

The max number of text parts is 3 by default. You can change it by
``max_n_results`` option:

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_max_n_results.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             { \
..                                "default_open_tag": "[", \
..                                "default_close_tag": "]", \
..                                "max_n_results": 1 \
..                             })' \
..   --match_columns content \
..   --query "fast performance"

It returns only one snippet because ``"max_n_results": 1`` is specified.

The max size of a text part is 200byte by default. The unit is bytes
not characters. The size doesn't include inserted ``[`` and ``[``. You
can change it by ``width`` option:

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_width.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             { \
..                                "default_open_tag": "[", \
..                                "default_close_tag": "]", \
..                                "width": 50 \
..                             })' \
..   --match_columns content \
..   --query "fast performance"

You can detect snippet delimiter with regular expression by
``delimiter_regexp`` option. You can use ``\.\s*`` to use only text in
the target sentence. Note that you need to escape ``\`` in string:

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_delimiter_regexp.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             { \
..                                "default_open_tag": "[", \
..                                "default_close_tag": "]", \
..                                "delimiter_regexp": "\\\\.\\\\s*" \
..                             })' \
..   --match_columns content \
..   --query "fast performance"

You can see the detected delimiters (``.`` and following white spaces)
aren't included in the result snippets. This is intentional behavior.

You can specify keywords explicitly instead of extracting keywords
from the current condition:

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_keywords.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             "fast", \
..                             "performance", \
..                             { \
..                                "default_open_tag": "[", \
..                                "default_close_tag": "]" \
..                             })'

This ``snippet`` returns two snippets for the first record and
``null`` for the second record. Because the second record doesn't have
any specified keywords.

You can specify open tag and close tag for each keyword:

.. groonga-command
.. include:: ../../example/reference/functions/snippet/usage_tags.log
.. select Documents \
..   --output_columns 'snippet(content, \
..                             "fast", "[", "]", \
..                             "performance", "(", ")")'

This ``snippet`` surrounds ``fast`` with ``[`` and ``]]`` and
``performance`` with ``(`` and ``)``.

TODO: ``html_escape`` option and so on

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

TODO

Optional parameters
^^^^^^^^^^^^^^^^^^^

TODO

.. _snippet-max-n-results:

``max_n_results``
"""""""""""""""""

TODO

.. _snippet-width:

``width``
"""""""""

TODO

Return value
------------

This function returns an array of string or ``null``. If This function
can't find any snippets, it returns ``null``.

An element of array is a snippet::

  [SNIPPET1, SNIPPET2, ...]

A snippet includes one or more keywords. The max byte size of a
snippet except open tag and close tag is 200byte. The unit isn't the
number of characters.

You can change this by :ref:`snippet-width` option.

The array size is larger than or equal to 1 and less than or equal
to 3.

You can change this by :ref:`snippet-max-n-results` option.

See also
--------

* :doc:`snippet_html`
* :doc:`/reference/commands/select`
