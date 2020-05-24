.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: functions_html_untag

``html_untag``
==============

Summary
-------

``html_untag`` strips HTML tags from HTML and outputs plain text.

``html_untag`` is used in ``--output_columns`` described at
:ref:`select-output-columns`.

Syntax
------

``html_untag`` requires only one argument. It is ``html``.

::

  html_untag(html)

Requirements
------------

``html_untag`` requires Groonga 3.0.5 or later.

``html_untag`` requires :doc:`/reference/command/command_version` 2 or
later.

Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/html_untag/usage_setup_schema.log
.. table_create WebClips TABLE_HASH_KEY ShortText
.. column_create WebClips content COLUMN_SCALAR ShortText

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/html_untag/usage_setup_data.log
.. load --table WebClips
.. [
.. {"_key": "http://groonga.org", "content": "groonga is <span class='emphasize'>fast</span>"},
.. {"_key": "http://mroonga.org", "content": "mroonga is <span class=\"emphasize\">fast</span>"},
.. ]

Here is the simple usage of ``html_untag`` function which strips HTML tags from content of column.

.. groonga-command
.. include:: ../../example/reference/functions/html_untag/usage_html_untag.log
.. select WebClips --output_columns "html_untag(content)" --command_version 2

When executing the above query, you can see "span" tag with "class" attribute is stripped.
Note that you must specify ``--command_version 2`` to use ``html_untag`` function.

Parameters
----------

There is only one required parameter.

``html``
^^^^^^^^

Specifies HTML text to be untagged.

Return value
------------

``html_untag`` returns plain text which is stripped HTML tags from HTML text.
