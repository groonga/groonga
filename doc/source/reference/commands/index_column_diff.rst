.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_index_column_diff

``index_column_diff``
=====================

Summary
-------

.. versionadded:: 9.0.1

``index_column_diff`` command check where indexes are broken or not.

We can found already a broken index by this command.
Normally, we don't found it unless Groonga refer, delete, or update it.
However, it is possible that Groonga crashes or returns wrong search
results by using it.
it make us want to found it in advance.
This command useful in this case.

.. note::

   This command may use many memory and execution time depending on the size of the target index.
   Also, if we stop in the middle of execution of this command, the target index may break.
   Therefore, we suggest that we don't execute this command on active system, but execute
   this command on standby system.

Syntax
------

This command takes two parameters.
All parameters are required::

  index_column_diff table index_column

Usage
-----

Here is an example to check a index column in the database:

.. groonga-command
.. include:: ../../example/reference/commands/index_column_diff/index_column.log
.. table_create Data TABLE_HASH_KEY ShortText
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenNgram \
..   --normalizer NormalizerNFKC130
.. load --table Data
.. [
.. {"_key": "Hello World"},
.. {"_key": "Hello Groonga"}
.. ]
.. column_create \
..   --table Terms \
..   --name data_index \
..   --flags COLUMN_INDEX|WITH_POSITION \
..   --type Data \
..   --source _key
.. truncate Terms.data_index
.. load --table Data
.. [
.. {"_key": "Good-by World"},
.. {"_key": "Good-by Groonga"}
.. ]
.. index_column_diff Terms data_index

Parameters
----------

This section describes all parameters.

``table``
"""""""""

Specifies the name of a table include check target of the index column.

``index_column``
""""""""""""""""

Specifies the name of check target of the index column.

Return value
------------

``index_column_diff`` command returns result of check indexes::

  [HEADER, CHECK_RESULT]

``HEADER``

See :doc:`/reference/command/output_format` about ``HEADER``.

``CHECK_RESULT``

This command returns the result of compression between the current
value of the index column and the result of tokenize when this command
execute as below::

    {
      "token": {
        "id": TOKEN_ID,
        "value": TOKEN_VALUE
      },
      "remains": [
        {
	  "record_id": RECORD_ID
	}
      ],
      "missings": [
        {
          "record_id": RECORD_ID,
          "position": POSITION
        }
      ]
    }

If there are something in ``remains``, a token that Groonga
was supposed to delete is remaining in a index.

If there are something in ``missing``, a token that Groonga
is supposing to remain in a index has been deleted from the index.

``index_column_diff`` returns nothing as below When indexes haven't broken::

  index_column_diff --table table --name index_column
  [[0,0.0,0.0],[]]

``TOKEN_ID``
""""""""""""

``TOKEN_ID`` is id of a broken token.

``TOKEN_VALUE``
"""""""""""""""

``TOKEN_VALUE`` is value of a broken token.

``RECORD_ID``
"""""""""""""

``RECORD_ID`` is id of a record include a broken token.

``POSITION``
""""""""""""

``POSITION`` is appearing position of a broken token.
