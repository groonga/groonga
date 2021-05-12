.. -*- rst -*-

Summary
=======

Groonga has token filter module that some processes tokenized token.

Token filter module can be added as a plugin.

You can customize tokenized token by registering your token filters plugins to Groonga.

A table can have zero or more token filters. You can attach token
filters to a table by :ref:`table-create-token-filters` option in
:doc:`/reference/commands/table_create`.

Here is an example ``table_create`` that uses ``TokenFilterStopWord``
token filter module:

.. groonga-command
.. database: token_filters_example
.. include:: ../../example/reference/token_filters/example-table-create.log
.. plugin_register token_filters/stop_word
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto \
..   --token_filters TokenFilterStopWord

See also
--------

* :doc:`/reference/commands/table_create`
