.. -*- rst -*-

.. groonga-command
.. database: regular_expression

Regular expression
==================

Summary
-------

.. note::

   Regular expression support is an experimental feature.

.. versionadded:: 5.0.1

Groonga supports pattern match by regular expression. Regular
expression is widely used format to describe a pattern. Regular
expression is useful to represent complex pattern.

In most cases, pattern match by regular expression is evaluated as
sequential search. It'll be slow for many records and many texts.

In some cases, pattern match by regular expression can be evaluated
by index. It's very fast rather than sequential search. Patterns
that can be evaluated by index are described later.

.. versionadded:: 5.0.7

   Groonga normalizes match target text by :ref:`normalizer-auto`
   normalizer when Groonga doesn't use index for regular expression
   search. It means that regular expression that has upper case such
   as ``Groonga`` never match. Because :ref:`normalizer-auto`
   normalizer normalizes all alphabets to lower case. ``groonga``
   matches to both ``Groonga`` and ``groonga``.

   Why is match target text normalizered? It's for increasing index
   search-able patterns. If Groonga doesn't normalize match target
   text, you need to write complex regular expression such as
   ``[Dd][Ii][Ss][Kk]`` and ``(?i)disk`` for case-insensitive match.
   Groonga can't use index against complex regular expression.

   If you write ``disk`` regular expression for case-insensitive
   match, Groonga can search the pattern with index. It's fast.

   By full text search normally, Groonga normalize search keywords
   using the normalizer specified in a lexicon.
   By using regular expression search, Groonga doesn't normalize search
   keywords.
   Because the regular expression has specified meaning in uppercase and
   lowercase.

   So, if you regular expression search that doesn't use the index,
   we suggest that use :doc:`/reference/commands/normalize` command to
   normalize search keywords before a search.
   By using :doc:`/reference/commands/normalize` command, you don't have
   to need to think about how to normalize search keywords.

   You may feel the behavior is strange. But fast search based on this
   behavior will help you.

There are many regular expression syntaxes. Groonga uses the same
syntax in Ruby. Because Groonga uses the same regular expression
engine as Ruby. The regular expression engine is `Onigmo
<https://github.com/k-takata/Onigmo/>`_. Characteristic difference
with other regular expression syntax is ``^`` and ``$``. The regular
expression syntax in Ruby, ``^`` means the beginning of line and ``$``
means the end of line. ``^`` means the beginning of text and ``$``
means the end of text in other most regular expression syntaxes. The regular
expression syntax in Ruby uses ``\A`` for the beginning of text and
``\z`` for the end of text.

.. versionadded:: 5.0.6

   Groonga uses multiline mode since 5.0.6. It means that ``.``
   matches on ``\n``.

   But it's meaningless. Because ``\n`` is removed by
   :ref:`normalizer-auto` normalizer.

You can use regular expression in :ref:`select-query` and
:ref:`select-filter` options of :doc:`/reference/commands/select`
command.

Usage
-----

Here are a schema definition and sample data to show usage. There is
only one table, ``Logs``. ``Logs`` table has only ``message``
column. Log messages are stored into the ``message`` column.

.. groonga-command
.. include:: ../example/reference/regular_expression/usage_setup.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs message COLUMN_SCALAR Text
..
.. load --table Logs
.. [
.. {"message": "host1:[error]: No memory"},
.. {"message": "host1:[warning]: Remained disk space is less than 30%"},
.. {"message": "host1:[error]: Disk full"},
.. {"message": "host2:[error]: No memory"},
.. {"message": "host2:[info]: Shutdown"}
.. ]

Here is an example that uses regular expression in
:ref:`select-query`. You need to use
``${COLUMN}:~${REGULAR_EXPRESSION}`` syntax.

.. groonga-command
.. include:: ../example/reference/regular_expression/usage_query.log
.. select Logs --query 'message:~"disk (space|full)"'

Here is an example that uses regular expression in
:ref:`select-filter`. You need to use
``${COLUMN} @~ ${REGULAR_EXPRESSION}`` syntax.

.. groonga-command
.. include:: ../example/reference/regular_expression/usage_filter.log
.. select Logs --filter 'message @~ "disk (space|full)"'

.. _regular-expression-index:

Index
-----

Groonga can search records by regular expression with index. It's very
fast rather than sequential search.

But it doesn't support all regular expression patterns. It supports
only the following regular expression patterns. The patterns will be
increased in the future.

  * Literal only pattern such as ``disk``
  * The begging of text and literal only pattern such as ``\Adisk``
  * The end of text and literal only pattern such as ``disk\z``

You need to create an index for fast regular expression search. Here
are requirements of index:

  * Lexicon must be :ref:`table-pat-key` table.
  * Lexicon must use :ref:`token-regexp` tokenizer.
  * Index column must has ``WITH_POSITION`` flag.

Other configurations such as lexicon's normalizer are optional. You
can choose what you like. If you want to use case-insensitive search,
use :ref:`normalizer-auto` normalizer.

Here are recommended index definitions. In general, it's reasonable
index definitions.

.. groonga-command
.. include:: ../example/reference/regular_expression/index_definitions.log
.. table_create RegexpLexicon TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenRegexp \
..   --normalizer NormalizerAuto
.. column_create RegexpLexicon logs_message_index \
..   COLUMN_INDEX|WITH_POSITION Logs message

Now, you can use index for regular expression search. The following
regular expression can be evaluated by index because it uses only "the
beginning of text" and "literal".

.. groonga-command
.. include:: ../example/reference/regular_expression/search_by_index_query.log
.. select Logs --query message:~\\\\Ahost1

Here is an example that uses :ref:`select-filter` instead of
:ref:`select-query`. It uses the same regular expression as the
previous example.

.. groonga-command
.. include:: ../example/reference/regular_expression/search_by_index_filter.log
.. select Logs --filter 'message @~ "\\\\Ahost1:"'

``\`` escape will confuse you because there are some steps that
require escape between you and Groonga. Here are steps that require
``\`` escape:

  * Shell only when you pass Groonga command from command line the
    following::

      % groonga /tmp/db select Logs --filter '"message @~ \"\\\\Ahost1:"\"'

    ``--filter '"message @~ \"\\\\Ahost1:\""'`` is evaluated as the
    following two arguments by shell:

      * ``--filter``
      * ``"message @~ \"\\\\Ahost1:\""``

  * Groonga command parser only when you pass Groonga command by
    command line style (``COMMAND ARG1_VALUE ARG2_VALUE ...``) not
    HTTP path style
    (``/d/COMMAND?ARG1_NAME=ARG1_VALUE&ARG2_NAME=ARG3_VALUE``).

    ``"message @~ \"\\\\Ahost1:\""`` is evaluated as the following
    value by Groonga command parser:

      * ``message @~ "\\Ahost1:"``

  * :doc:`/reference/grn_expr` parser. ``\`` escape is required in both
    :doc:`/reference/grn_expr/query_syntax` and
    :doc:`/reference/grn_expr/script_syntax`.

    ``"\\Ahost1:"`` string literal in script syntax is evaluated as
    the following value:

      * ``\Ahost1``

    The value is evaluated as regular expression.

.. _regular-expression-syntax:

Syntax
------

This section describes about only commonly used syntaxes. See `Onigmo
syntax documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE>`_ for other
syntaxes and details.

.. _regular-expression-escape:

Escape
^^^^^^

In regular expression, there are the following special characters:

  * ``\``
  * ``|``
  * ``(``
  * ``)``
  * ``[``
  * ``]``
  * ``.``
  * ``*``
  * ``+``
  * ``?``
  * ``{``
  * ``}``
  * ``^``
  * ``$``

If you want to write pattern that matches these special character as
is, you need to escape them.

You can escape them by putting ``\`` before special character. Here
are regular expressions that match special character itself:

  * ``\\``
  * ``\|``
  * ``\(``
  * ``\)``
  * ``\[``
  * ``\]``
  * ``\.``
  * ``\*``
  * ``\+``
  * ``\?``
  * ``\{``
  * ``\}``
  * ``\^``
  * ``\$``

.. groonga-command
.. include:: ../example/reference/regular_expression/choice.log
.. select Logs --filter 'message @~ "\\\\[error\\\\]"'

If your regular expression doesn't work as you expected, confirm that
some special characters are used without escaping.

.. _regular-expression-choice:

Choice
^^^^^^

Choice syntax is ``A|B``. The regular expression matches when either
``A`` pattern or ``B`` pattern is matched.

.. groonga-command
.. include:: ../example/reference/regular_expression/choice.log
.. select Logs --filter 'message @~ "warning|info"'

.. caution::

   Regular expression that uses this syntax can't be evaluated by
   index.

.. _regular-expression-group:

Group
^^^^^

Group syntax is ``(...)``. Group provides the following features:

  * Back reference
  * Scope reducing

You can refer matched groups by ``\n`` (``n`` is the group number)
syntax. For example, ``e(r)\1o\1`` matches ``error``. Because ``\1``
is replaced with match result (``r``) of the first group ``(r)``.

.. groonga-command
.. include:: ../example/reference/regular_expression/group_back_reference.log
.. select Logs --filter 'message @~ "e(r)\\\\1o\\\\1"'

You can also use more powerful back reference features. See `"8. Back
reference" section in Onigmo documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE#L302>`_ for
details.

Group syntax reduces scope. For example, ``\[(warning|info)\]``
reduces choice syntax scope. The regular expression matches
``[warning]`` and ``[info]``.

.. groonga-command
.. include:: ../example/reference/regular_expression/group_scope_reducing.log
.. select Logs --filter 'message @~ "\\\\[(warning|info)\\\\]"'

You can also use more powerful group related features. See
`"7. Extended groups" section in Onigmo documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE#L225>`_ for
details.

.. caution::

   Regular expression that uses this syntax can't be evaluated by
   index.

.. _regular-expression-character-class:

Character class
^^^^^^^^^^^^^^^

Character class syntax is ``[...]``. Character class is useful to
specify multiple characters to be matched.

For example, ``[12]`` matches ``1`` or ``2``.

.. groonga-command
.. include:: ../example/reference/regular_expression/character_class_characters.log
.. select Logs --filter 'message @~ "host[12]"'

You can specify characters by range. For example, ``[0-9]`` matches
one digit.

.. groonga-command
.. include:: ../example/reference/regular_expression/character_class_range.log
.. select Logs --filter 'message @~ "[0-9][0-9]%"'

You can also use more powerful character class related features. See
`"6. Character class" section in Onigmo documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE#L164>`_ for
details.

.. caution::

   Regular expression that uses this syntax can't be evaluated by
   index.

.. _regular-expression-anchor:

Anchor
^^^^^^

There are the following commonly used anchor syntaxes. Some anchors
can be evaluated by index.

.. list-table::
   :header-rows: 1

   * - Anchor
     - Description
     - Index ready
   * - ``^``
     - The beginning of line
     - o
   * - ``$``
     - The end of line
     - x
   * - ``\A``
     - The beginning of text
     - o
   * - ``\z``
     - The end of text
     - x

Here is an example that uses ``\z``.

.. groonga-command
.. include:: ../example/reference/regular_expression/anchor_z.log
.. select Logs --filter 'message @~ "%\\\\z"'

You can also use more anchors. See `"5. Anchors" section in Onigmo
documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE#L152>`_ for
details.

.. caution::

   Regular expression that uses this syntax except ``\A`` and ``\z``
   can't be evaluated by index.

.. _regular-expression-quantifier:

Quantifier
^^^^^^^^^^

There are the following commonly used quantifier syntaxes.

.. list-table::
   :header-rows: 1

   * - Quantifier
     - Description
   * - ``?``
     - 0 or 1 time
   * - ``*``
     - 0 or more times
   * - ``+``
     - 1 or more times

For example, ``er+or`` matches ``error``, ``errror`` and so on.

.. groonga-command
.. include:: ../example/reference/regular_expression/quantifier_plus.log
.. select Logs --filter 'message @~ "er+or"'

You can also use more quantifiers. See `"4. Quantifier" section in Onigmo
documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE#L119>`_ for
details.

.. caution::

   Regular expression that uses this syntax can't be evaluated by
   index.

Others
^^^^^^

There are more syntaxes. If you're interested in them, see `Onigmo
documentation
<https://github.com/k-takata/Onigmo/blob/master/doc/RE>`_ for
details. You may be interested in "character type" and "character"
syntaxes.
