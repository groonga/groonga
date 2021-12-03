.. -*- rst -*-

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
     - TODO
   * - ``WITH_TYPES``
     - TODO
   * - ``WITH_CHECKS``
     - If we specify this flag, Groonga output position of character before normalizing.
       Note that these positions of character before normalizing are a relative position
       against a previous character.
   * - ``REMOVE_TOKENIZED_DELIMITER``
     - TODO

Here is an example that uses ``REMOVE_BLANK``.

TODO

Here is an example that uses ``WITH_TYPES``.

TODO

Here is an example that uses ``WITH_CHECKS``.

.. groonga-command
.. include:: ../../example/reference/commands/normalize/normalizer_auto_with_checks.log
.. normalize NormalizerAuto "　A　　B　　　C" WITH_CHECKS

Here is an example that uses ``REMOVE_TOKENIZED_DELIMITER``.

TODO

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
