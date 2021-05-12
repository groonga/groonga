.. -*- rst -*-

.. groonga-command
.. database: commands_ruby_eval

``ruby_eval``
=============

Summary
-------

``ruby_eval`` command evaluates Ruby script and returns the result.

Syntax
------

This command takes only one required parameter::

  ruby_eval script

Usage
-----

You can execute any scripts which mruby supports by calling ``ruby_eval``.

Here is an example that just calculate ``1 + 2`` as Ruby script.

.. groonga-command
.. include:: ../../example/reference/commands/ruby_eval/calc.log
.. plugin_register ruby/eval
.. ruby_eval "1 + 2"

Register ``ruby/eval`` plugin to use ``ruby_eval`` command in advance.

Note that ``ruby_eval`` is implemented as an experimental plugin,
and the specification may be changed in the future.

Parameters
----------

This section describes all parameters.

``script``
""""""""""

Specifies the Ruby script which you want to evaluate.

Return value
------------

``ruby_eval`` returns the evaluated result with metadata such as
exception information (Including metadata isn't implemented yet)::

  [HEADER, {"value": EVALUATED_VALUE}]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``EVALUATED_VALUE``

  ``EVALUATED_VALUE`` is the evaludated value of ``ruby_script``.

  ``ruby_eval`` supports only a number for evaluated value for now.
  Supported types will be increased in the future.

See also
--------

