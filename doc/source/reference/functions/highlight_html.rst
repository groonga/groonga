.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: functions_highlight_html

``highlight_html``
==================

.. versionadded:: 4.0.5

Summary
-------

``highlight_html`` tags target text. It can use to highlight the search
keywords. The tagged text are prepared for embedding HTML. Special
characters such as ``<`` and ``>`` are escapsed as ``&lt;`` and ``&gt;``.
Keyword is surrounded with ``<span class="keyword">`` and ``</span>``.
For example, a tagged text of ``I am a groonga user. <3`` for keyword
``groonga`` is ``I am a <span class="keyword">groonga</span> user. &lt;3``.

Syntax
------

This function has only one parameter::

  highlight_html(text)

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_html/usage_setup.log
.. table_create Entries TABLE_NO_KEY
.. column_create Entries body COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body
.. load --table Entries
.. [
.. {"body": "Mroonga is a ＭｙＳＱＬ storage engine based on Groonga. <b>Rroonga</b> is a Ruby binding of Groonga."}
.. ]

``highlight_html`` can be used in only ``--output_columns`` in
:doc:`/reference/commands/select`.

``highlight_html`` requires :doc:`/reference/command/command_version` 2
or later.

You also need to specify ``--query`` and/or ``--filter``. Keywords are
extracted from ``--query`` and ``--filter`` arguments.

The following example uses ``--query "groonga mysql"``. In this case,
``groonga`` and ``mysql`` are used as keywords.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_html/usage_basic.log
.. select Entries --output_columns --match_columns body --query 'groonga mysql' --output_columns 'highlight_html(body)' --command_version 2

The text are scanned by the keywords for tagging after they are normalized
by ``NormalizerAuto`` normalizer.

``--query "groonga mysql"`` matches to only the first record's body.
``highlight_html(body)`` surrounds the keywords ``groonga`` or ``mysql``
contained in the text with ``<span class="keyword">`` and ``</span>``.

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_html/usage_string_literal.log
.. select Entries --output_columns 'highlight_html("Groonga is very fast fulltext search engine.")' --command_version 2 --match_columns body --query "groonga"

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

``text``
""""""""

The text to be highlighted in HTML.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

``highlight_html`` returns a tagged string or ``null``. If
``highlight_html`` can't find any keywords, it returns ``null``.

See also
--------

* :doc:`/reference/commands/select`
* :doc:`/reference/functions/highlight_full`
