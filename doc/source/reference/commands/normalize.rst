.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_normalize

``normalize``
=============

.. note::

   This command is an experimental feature.

   This command may be changed in the future.

Summary
-------

``normalize`` command normalizes text by the specified normalizer.

There is no need to create table to use ``normalize`` command.
It is useful for you to check the results of normalizer.

Syntax
--------

This command takes three parameters.

``normalizer`` and ``string`` are required. Others are optional::

  normalize normalizer
            string
            [flags=NONE]

Usage
-----

Here is a simple example of ``normalize`` command.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_auto_ascii.log
.. normalize NormalizerAuto "aBcDe 123"

Parameters
----------

This section describes parameters of ``normalizer``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters, ``normalizer`` and ``string``.

``normalizer``
""""""""""""""

Specifies the normalizer name. ``normalize`` command uses the
normalizer that is named ``normalizer``.

See :doc:`/reference/normalizers` about built-in normalizers.

Here is an example to use built-in ``NormalizerAuto`` normalizer.

TODO

If you want to use other normalizers, you need to register additional
normalizer plugin by :doc:`register` command. For example, you can use
MySQL compatible normalizer by registering `groonga-normalizer-mysql
<https://github.com/groonga/groonga-normalizer-mysql>`_.

``string``
""""""""""

Specifies any string which you want to normalize.

If you want to include spaces in ``string``, you need to quote
``string`` by single quotation (``'``) or double quotation (``"``).

Here is an example to use spaces in ``string``.

TODO

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``flags``
"""""""""

Specifies a normalization customize options. You can specify
multiple options separated by "``|``". For example,
``REMOVE_BLANK|WITH_TYPES``.

Here are available flags.

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
   * - ``NONE``
     - Just ignored.
   * - ``REMOVE_BLANK``
     - Output remove blanks from characters of beforing normalize.
   * - ``WITH_TYPES``
     - Output kind of character after normalize.
   * - ``WITH_CHECKS``
     - Output where was position of character before normalize.
   * - ``REMOVE_TOKENIZED_DELIMITER``
     - Output remove tokenized delimiter(U+FFFE) from character of beforing normalize.

Here is an example that uses ``REMOVE_BLANK``.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_auto_remove_blank.log
.. normalize --normalizer NormalizerAuto --string "abc 123" --flags REMOVE_BLANK

Here is an example that uses ``WITH_TYPES``.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_nfkc100_with_types.log
.. normalize --normalizer 'NormalizerNFKC100("unify_to_romaji", true)' --string "あいうえお" --flags WITH_TYPES

Here is an example that uses ``WITH_CHECKS``.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_auto_with_checks.log
.. normalize --normalizer NormalizerAuto --string "㌖" --flags WITH_CHECKS

Here is an example that uses ``REMOVE_TOKENIZED_DELIMITER``.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_auto_remove_tokenized_delimiter.log
.. normalize --normalizer NormalizerAuto --string "a￾b￾c" --flags REMOVE_TOKENIZED_DELIMITER

Return value
------------

::

  [HEADER, normalized_text]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``normalized_text``

  ``normalized_text`` is an object that has the following attributes.

  .. list-table::
     :header-rows: 1

     * - Name
       - Description
     * - ``normalized``
       - The normalized text.
     * - ``types``
       - An array of types of the normalized text. The N-th ``types`` shows
         the type of the N-th character in ``normalized``.

See also
--------

* :doc:`/reference/normalizers`
