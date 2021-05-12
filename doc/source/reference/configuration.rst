.. -*- rst -*-

Configuration
=============

.. versionadded:: 5.1.2

Groonga can manage configuration items in each database. These
configuration items are persistent. It means that these configuration
items are usable after a Groonga process exits.

Summary
-------

You can change some Groonga behaviors such as :doc:`/spec/search` by
some ways such as request parameter
(:ref:`select-match-escalation-threshold`) and build parameter
(:ref:`install-configure-with-match-escalation-threshold`).

Configuration is one of these ways. You can change some Groonga
behaviors per database by configuration.

A configuration item consists of key and value. Both of key and value
are string. The max key size is 4KiB. The max value size is 4091B (=
4KiB - 5B).

You can set a configuration item by
:doc:`/reference/commands/config_set`.

You can get a configuration item by
:doc:`/reference/commands/config_get`.

You can delete a configuration item by
:doc:`/reference/commands/config_delete`.

You can confirm all configuration items by
:doc:`/reference/commands/dump`.

.. _configuration-commands:

Commands
--------

  * :doc:`/reference/commands/config_delete`
  * :doc:`/reference/commands/config_get`
  * :doc:`/reference/commands/config_set`
