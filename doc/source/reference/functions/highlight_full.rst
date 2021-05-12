.. -*- rst -*-

.. groonga-command
.. database: functions_highlight_full

``highlight_full``
==================

.. versionadded:: 4.0.5

Summary
-------

``highlight_full`` tags target text. It can use to highlight the search
keyword. It can specify use/not use HTML escape, the normalizer name and
change the tag for each keyword.

Syntax
------

``highlight_full`` has required parameter and optional parameter::

  highlight_full(column, normalizer_name, use_html_escape,
                 keyword1, open_tag1, close_tag1,
                 ...
                 [keywordN, open_tagN, close_tagN])

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_full/usage_setup.log
.. table_create Entries TABLE_NO_KEY
.. column_create Entries body COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Terms document_index COLUMN_INDEX|WITH_POSITION Entries body
.. load --table Entries
.. [
.. {"body": "Mroonga is a ＭｙＳＱＬ storage engine based on Groonga. <b>Rroonga</b> is a Ruby binding of Groonga."}
.. ]

``highlight_full`` can be used in only ``--output_columns`` in
:doc:`/reference/commands/select` before version 10.0.6 (exclusive).
However, it can be also used in ``--output_columns`` in
:doc:`/reference/commands/logical_select` since version 10.0.6.

``highlight_full`` requires :doc:`/reference/command/command_version` 2
or later.

The following example uses HTML escape and normalzier is ``NormalizeAuto``.
It specifies the tags ``<span class="keyword1">`` and ``</span>`` of the
keyword ``groonga``, and the tags ``<span class="keyword2">`` and ``</span>``
of the keyword ``mysql``.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_full/usage_basic.log
.. select Entries --output_columns 'highlight_full(body, "NormalizerAuto", true, "Groonga", "<span class=\\"keyword1\\">", "</span>", "mysql", "<span class=\\"keyword2\\">", "</span>")' --command_version 2

The text are scanned by the keywords for tagging after they are normalized
by ``NormalizerAuto`` normalizer.

``--query "groonga mysql"`` matches to the first record's body.
``highight_full`` surrounds the keywords ``groonga`` contained in the text
with ``<span class="keyword1">`` and ``</span>``, and the keywords ``mysql``
contained in the text with with ``<span class="keyword2">`` and ``</span>``.

Special characters such as ``<`` and ``>`` are escapsed as ``&lt;`` and
``&gt;``.

You can specify string literal instead of column.

.. groonga-command
.. include:: ../../example/reference/functions/highlight_full/usage_string_literal.log
.. select Entries --output_columns 'highlight_full("Groonga is very fast fulltext search engine.", "NormalizerAuto", true, "Groonga", "<span class=\\"keyword1\\">", "</span>", "mysql", "<span class=\\"keyword2\\">", "</span>")' --command_version 2 --match_columns body --query "groonga"

Parameters
----------
There are three required parameters, ``column``, ``normalizer_name`` and ``use_html_escape``.
There are three or over optional parameters, ``keywordN``, ``open_tagN`` and ``end_tagN``.

``column``
^^^^^^^^^^

Specifies a column of the table.

``normalizer_name``
^^^^^^^^^^^^^^^^^^^

Specifies a normalizer name.

``use_html_escape``
^^^^^^^^^^^^^^^^^^^

Specifies use or not use HTML escape. If it is ``true`` , use HTML escape.
If it is ``false`` , not use HTML escape.

``keywordN``
^^^^^^^^^^^^

Specifies a keyword for tagging.
You can specify multiple keywords for each three arguments.

``open_tagN``
^^^^^^^^^^^^^

Specifies a open tag.
You can specify multiple open tags for each three arguments.

``close_tagN``
^^^^^^^^^^^^^^

Specifies a close tag.
You can specify multiple close tags for each three arguments.

Return value
------------

``highlight_full`` returns a tagged string or ``null``. If
``highlight_full`` can't find any keywords, it returns ``null``.

See also
--------

* :doc:`/reference/commands/select`
* :doc:`/reference/commands/logical_select`
* :doc:`/reference/functions/highlight_html`
