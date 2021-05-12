.. -*- rst -*-

.. groonga-command
.. database: commands_normalizer_list

``normalizer_list``
===================

Summary
-------

``normalizer_list`` command lists normalizers in a database.


Syntax
------

This command takes no parameters::

  normalizer_list

Usage
-----

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/commands/normalizer_list/simple_example.log
.. normalizer_list

It returns normalizers in a database.

Return value
------------

``normalizer_list`` command returns normalizers. Each normalizers has an attribute
that contains the name. The attribute will be increased in the feature::

  [HEADER, normalizers]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``normalizers``

  ``normalizers`` is an array of normalizer. Normalizer is an object that has the following
  attributes.

  .. list-table::
     :header-rows: 1

     * - Name
       - Description
     * - ``name``
       - Normalizer name.

See also
--------

* :doc:`/reference/normalizers`
* :doc:`/reference/commands/normalize`
