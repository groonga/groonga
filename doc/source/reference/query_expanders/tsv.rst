.. -*- rst -*-

QueryExpanderTSV
================

Summary
-------

``QueryExpanderTSV`` is a query expander plugin that reads synonyms
from TSV (Tab Separated Values) file. This plugin provides poor
feature than the embedded query expansion feature. For example, it
doesn't support word normalization. But it may be easy to use because
you can manage your synonyms by TSV file. You can edit your synonyms
by spreadsheet application such as Excel. With the embedded query
expansion feature, you manage your synonyms by Groonga's table.

Install
-------

You need to register ``query_expanders/tsv`` as a plugin before you
use ``QueryExpanderTSV``::

  plugin_register query_expanders/tsv

Usage
-----

You just add ``--query_expander QueryExpanderTSV`` parameter to
``select`` command::

  select --query "QUERY" --query_expander QueryExpanderTSV

If ``QUERY`` has registered synonyms, they are expanded. For example,
there are the following synonyms.

+----------------------------+------------------------+----------------------+
|            word            |       synonym 1        |      synonym 2       |
+============================+========================+======================+
|          groonga           |        groonga         |        Senna         |
+----------------------------+------------------------+----------------------+
|          mroonga           |        mroonga         |    groonga MySQL     |
+----------------------------+------------------------+----------------------+

The table means that ``synonym 1`` and ``synonym 2`` are synonyms of
``word``. For example, ``groonga`` and ``Senna`` are synonyms of
``groonga``. And ``mroonga`` and ``groonga MySQL`` are synonyms of
``mroonga``.

Here is an example of query expnasion that uses ``groonga`` as query::

  select --query "groonga" --query_expander QueryExpanderTSV

The above command equals to the following command::

  select --query "groonga OR Senna" --query_expander QueryExpanderTSV

Here is another example of query expnasion that uses ``mroonga
search`` as query::

  select --query "mroonga search" --query_expander QueryExpanderTSV

The above command equals to the following command::

  select --query "(mroonga OR (groonga MySQL)) search" --query_expander QueryExpanderTSV

It is important that registered words (``groonga`` and ``mroonga``)
are only expanded to synonyms and not registered words (``search``)
are not expanded. Query expansion isn't occurred
recursively. ``groonga`` is appeared in ``(mroonga OR (groonga
MySQL))`` as query expansion result but it isn't expanded.

Normally, you need to include ``word`` itself into synonyms. For
example, ``groonga`` and ``mroonga`` are included in synonyms of
themselves. If you want to ignore ``word`` itself, you don't include
``word`` itself into synonyms. For example, if you want to use query
expansion as spelling correction, you should use the following
synonyms.

+----------------------------+------------------------+
|            word            |        synonym         |
+============================+========================+
|           gronga           |        groonga         |
+----------------------------+------------------------+

``gronga`` in ``word`` has a typo. A ``o`` is missing. ``groonga`` in
``synonym`` is the correct word.

Here is an example of using query expnasion as spelling correction::

  select --query "gronga" --query_expander QueryExpanderTSV

The above command equals to the following command::

  select --query "groonga" --query_expander QueryExpanderTSV

The former command has a typo in ``--query`` value but the latter
command doesn't have any typos.

TSV File
--------

Synonyms are defined in TSV format file. This section describes about
it.

Location
^^^^^^^^

The file name should be ``synonyms.tsv`` and it is located at
configuration directory. For example, ``/etc/groonga/synonyms.tsv`` is
a TSV file location. The location is decided at build time.

You can change the location by environment variable
``GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE`` at run time::

  % env GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE=/tmp/synonyms.tsv groonga

With the above command, ``/tmp/synonyms.tsv`` file is used.

Format
^^^^^^

You can define zero or more synonyms in a TSV file. You define a
``word`` and ``synonyms`` pair by a line. ``word`` is expanded to
``synonyms`` in ``--query`` value. ``Synonyms`` are combined by
``OR``. For example, ``groonga`` and ``Senna`` synonyms are expanded
as ``groonga OR Senna``.

The first column is ``word`` and the rest columns are ``synonyms`` of
the ``word``. Here is a sample line for ``word`` is ``groonga`` and
``synonyms`` are ``groonga`` and ``Senna``. ``(TAB)`` means a tab
character (``U+0009``)::

  groonga(TAB)groonga(TAB)Senna

Comment line is supported. Lines that start with ``#`` are ignored.
Here is an example for comment line. ``groonga`` line is ignored as
comment line::

  #groonga(TAB)groonga(TAB)Senna
  mroonga(TAB)mroonga(TAB)groonga MySQL

Limitation
----------

You need to restart groonga to reload your synonyms. TSV file is
loaded only at the plugin load time.

See also
--------

  * :ref:`select-query-expansion`
