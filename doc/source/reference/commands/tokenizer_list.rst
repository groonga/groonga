.. -*- rst -*-

.. groonga-command
.. database: commands_tokenizer_list

``tokenizer_list``
==================

Summary
-------

``tokenizer_list`` command lists tokenizers in a database.

Syntax
------

This command takes no parameters::

  tokenizer_list

Usage
-----

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/commands/tokenizer_list/simple_example.log
.. tokenizer_list

It returns tokenizers in a database.

Return value
------------

``tokenizer_list`` command returns tokenizers. Each tokenizers has an attribute
that contains the name. The attribute will be increased in the feature::

  [HEADER, tokenizers]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``tokenizers``

  ``tokenizers`` is an array of tokenizer. Tokenizer is an object that has the following
  attributes.

  .. list-table::
     :header-rows: 1

     * - Name
       - Description
     * - ``name``
       - Tokenizer name.

See also
--------

* :doc:`/reference/tokenizers`
* :doc:`/reference/commands/tokenize`
