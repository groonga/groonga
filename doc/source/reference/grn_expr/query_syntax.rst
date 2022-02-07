.. -*- rst -*-

.. groonga-command
.. database: reference_grn_expr_query_syntax

Query syntax
============

Query syntax is a syntax to specify search condition for common Web
search form. It is similar to the syntax of Google's search form. For
example, ``word1 word2`` means that groonga searches records that
contain both ``word1`` and ``word2``. ``word1 OR word2`` means that
groonga searches records that contain either ``word1`` or ``word2``.

Query syntax consists of ``conditional expression``, ``combind
expression`` and ``assignment expression``. Nomrally ``assignment
expression`` can be ignored. Because ``assignment expression`` is
disabled in ``--query`` option of :doc:`/reference/commands/select`. You can use
it if you use groonga as library and customize query syntax parser
options.

``Conditional expression`` specifies an condition. ``Combinded
expression`` consists of one or more ``conditional expression``,
``combined expression`` or ``assignment expression``. ``Assignment
expression`` can assigns a column to a value.

Sample data
-----------

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/setup.log
.. table_create Entries TABLE_PAT_KEY ShortText
.. column_create Entries content COLUMN_SCALAR Text
.. column_create Entries n_likes COLUMN_SCALAR UInt32
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Terms entries_key_index COLUMN_INDEX|WITH_POSITION Entries _key
.. column_create Terms entries_content_index COLUMN_INDEX|WITH_POSITION Entries content
.. load --table Entries
.. [
.. {"_key":    "The first post!",
..  "content": "Welcome! This is my first post!",
..  "n_likes": 5},
.. {"_key":    "Groonga",
..  "content": "I started to use Groonga. It's very fast!",
..  "n_likes": 10},
.. {"_key":    "Mroonga",
..  "content": "I also started to use Mroonga. It's also very fast! Really fast!",
..  "n_likes": 15},
.. {"_key":    "Good-bye Senna",
..  "content": "I migrated all Senna system!",
..  "n_likes": 3},
.. {"_key":    "Good-bye Tritonn",
..  "content": "I also migrated all Tritonn system!",
..  "n_likes": 3}
.. ]

There is a table, ``Entries``, for blog entries. An entry has title,
content and the number of likes for the entry. Title is key of
``Entries``. Content is value of ``Entries.content`` column. The
number of likes is value of ``Entries.n_likes`` column.

``Entries._key`` column and ``Entries.content`` column are indexed
using ``TokenBigram`` tokenizer. So both ``Entries._key`` and
``Entries.content`` are fulltext search ready.

OK. The schema and data for examples are ready.

Escape
------

There are special characters in query syntax. To use a special
character as itself, it should be escaped by prepending ``\``. For
example, ``"`` is a special character. It is escaped as ``\"``.

Here is a special character list:

  * ``[space]`` (escaped as ``[backslash][space]``) (You should
    substitute ``[space]`` with a white space character that is 0x20
    in ASCII and ``[backslash]`` with ``\\``.)
  * ``"`` (escaped as ``\"``)
  * ``(`` (escaped as ``\(``)
  * ``)`` (escaped as ``\)``)
  * ``\`` (escaped as ``\\``)

You can use quote instead of escape special characters except ``\``
(backslash). You need to use backslash for escaping backslash like
``\\`` in quote.

Quote syntax is ``"..."``. You need escape ``"`` as
``\"`` in ``"..."`` quote syntax. For example, ``You say "Hello Alice!"`` can be
quoted ``"You say \"Hello Alice!\""``.

In addition ``'...'`` isn't  available in query syntax.

.. note::

   There is an important point which you have to care. The ``\``
   (backslash) character is interpreted by command line shell. So if
   you want to search ``(`` itself for example, you need to escape
   twice (``\\(``) in command line shell.  The command line shell
   interprets ``\\(`` as ``\(``, then pass such a literal to
   Groonga. Groonga regards ``\(`` as ``(``, then search ``(`` itself
   from database. If you can't do intended search by Groonga, confirm
   whether special character is escaped properly.

.. _query-syntax-conditional-expression:

Conditional expression
----------------------

Here is available conditional expression list.

.. _query-syntax-full-text-search-condition:

Full text search condition
^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``keyword``.

``Full text search condition`` specifies a full text search condition
against the default match columns. Match columns are full text search
target columns.

You should specify the default match columns for full text
search. They can be specified by ``--match_columns`` option of
:doc:`/reference/commands/select`. If you don't specify the default match
columns, this conditional expression fails.

This conditional expression does full text search with
``keyword``. ``keyword`` should not contain any spaces. If ``keyword``
contains a space such as ``search keyword``, it means two full text
search conditions; ``search`` and ``keyword``. If you want to
specifies a keyword that contains one or more spaces, you can use
``phrase search condition`` that is described below.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_full_text_search.log
.. select Entries --match_columns content --query fast

The expression matches records that contain a word ``fast`` in
``content`` column value.

``content`` column is the default match column.

.. _query-syntax-phrase-search-condition:

Phrase search condition
^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``"search keyword"``.

``Phrase search condition`` specifies a phrase search condition
against the default match columns.

You should specify the default match columns for full text
search. They can be specified by ``--match_columns`` option of
:doc:`/reference/commands/select`. If you don't specify the default match
columns, this conditional expression fails.

This conditional expression does phrase search with ``search
keyword``. Phrase search searches records that contain ``search`` and
``keyword`` and those terms are appeared in the same order and
adjacent. Thus, ``Put a search keyword in the form`` is matched but
``Search by the keyword`` and ``There is a keyword. Search by it!``
aren't matched.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_phrase_search.log
.. select Entries --match_columns content --query '"I started"'

The expression matches records that contain a phrase ``I started`` in
``content`` column value. ``I also started`` isn't matched because
``I`` and ``started`` aren't adjacent.

``content`` column is the default match column.

.. _query-syntax-full-text-search-condition-with-explicit-match-column:

Full text search condition (with explicit match column)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:@keyword``.

It's similar to ``full text search condition`` but it doesn't require
the default match columns. You need to specify match column for the
full text search condition by ``column:`` instead of
``--match_columns`` option of :doc:`/reference/commands/select`.

This condtional expression is useful when you want to use two or more
full text search against different columns. The default match columns
specified by ``--match_columns`` option can't be specified multiple
times. You need to specify the second match column by this conditional
expression.

The different between ``full text search condition`` and ``full text
search condition (with explicit match column)`` is whether advanced
match columns are supported or not. ``Full text search condition``
supports advanced match columns but ``full text search condition (with
explicit match column)`` isn't supported. Advanced match columns has
the following features:

  * Weight is supported.
  * Using multiple columns are supported.
  * Using index column as a match column is supported.

See description of ``--match_columns`` option of
:doc:`/reference/commands/select` about them.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_full_text_search_with_explicit_match_column.log
.. select Entries --query content:@fast

The expression matches records that contain a word ``fast`` in
``content`` column value.

Phrase search condition (with explicit match column)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:@"search keyword"``.

It's similar to ``phrase search condition`` but it doesn't require the
default match columns. You need to specify match column for the phrase
search condition by ``column:`` instead of ``--match_columns`` option
of :doc:`/reference/commands/select`.

The different between ``phrase search condition`` and ``phrase search
condition (with explicit match column)`` is similar to between ``full
text search condition`` and ``full text search condition (with
explicit match column)``. ``Phrase search condition`` supports
advanced match columns but ``phrase search condition (with explicit
match column)`` isn't supported. See description of ``full text search
condition (with explicit match column)`` about advanced match columns.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_phrase_search_with_explicit_match_column.log
.. select Entries --query 'content:@"I started"'

The expression matches records that contain a phrase ``I started`` in
``content`` column value. ``I also started`` isn't matched because
``I`` and ``started`` aren't adjacent.

.. _query-syntax-prefix-search-condition:

Prefix search condition
^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:^value`` or ``value*``.

This conditional expression does prefix search with ``value``. Prefix
search searches records that contain a word that starts with ``value``.

You can use fast prefix search against a column. The column must be
indexed and index table must be patricia trie table
(``TABLE_PAT_KEY``) or double array trie table
(``TABLE_DAT_KEY``). You can also use fast prefix search against
``_key`` pseudo column of patricia trie table or double array trie
table. You don't need to index ``_key``.

Prefix search can be used with other table types but it causes all
records scan. It's not problem for small records but it spends more
time for large records.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_prefix_search.log
.. select Entries --query '_key:^Goo'

The expression matches records that contain a word that starts with
``Goo`` in ``_key`` pseudo column value. ``Good-bye Senna`` and
``Good-bye Tritonn`` are matched with the expression.

.. _query-syntax-suffix-search-condition:

Suffix search condition
^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:$value``.

This conditional expression does suffix search with ``value``. Suffix
search searches records that contain a word that ends with ``value``.

You can use fast suffix search against a column. The column must be
indexed and index table must be patricia trie table
(``TABLE_PAT_KEY``) with ``KEY_WITH_SIS`` flag. You can also use fast
suffix search against ``_key`` pseudo column of patricia trie table
(``TABLE_PAT_KEY``) with ``KEY_WITH_SIS`` flag. You don't need to
index ``_key``. We recommended that you use index column based fast
suffix search instead of ``_key`` based fast suffix search. ``_key``
based fast suffix search returns automatically registered
substrings. (TODO: write document about suffix search and link to it
from here.)

.. note::

   Fast suffix search can be used only for non-ASCII characters such
   as hiragana in Japanese. You cannot use fast suffix search for
   ASCII character.

Suffix search can be used with other table types or patricia trie
table without ``KEY_WITH_SIS`` flag but it causes all records
scan. It's not problem for small records but it spends more time for
large records.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example. It uses fast suffix search for hiragana in
Japanese that is one of non-ASCII characters.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_suffix_search.log
.. table_create Titles TABLE_NO_KEY
.. column_create Titles content COLUMN_SCALAR ShortText
.. table_create SuffixSearchTerms TABLE_PAT_KEY|KEY_WITH_SIS ShortText
.. column_create SuffixSearchTerms index COLUMN_INDEX Titles content
.. load --table Titles
.. [
.. {"content": "ぐるんが"},
.. {"content": "むるんが"},
.. {"content": "せな"},
.. {"content": "とりとん"}
.. ]
.. select Titles --query 'content:$んが'

The expression matches records that have value that ends with ``んが``
in ``content`` column value. ``ぐるんが`` and ``むるんが`` are matched
with the expression.

.. _query-syntax-near-search-condition:

Near search condition
^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``*N"token1 token2 ..."``.

This conditional expression does near search with ``token1``,
``token2`` and ``...``. Near search searches records that contain the
all specified tokens and there are at most 10 tokens between them. For
example, ``*N"a b c"`` matches ``a 1 2 3 4 5 b 6 7 8 9 10 c`` but
doesn't match ``a 1 2 3 4 5 b 6 7 8 9 10 11 c``:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_search_search.log
.. table_create NearTokens TABLE_NO_KEY
.. column_create NearTokens content COLUMN_SCALAR ShortText
.. table_create NearTokensTerms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenNgram \
..   --normalizer NormalizerNFKC130
.. column_create NearTokensTerms index COLUMN_INDEX|WITH_POSITION \
..   NearTokens content
.. load --table NearTokens
.. [
.. {"content": "a 1 2 3 4 5 b 6 7 8 9 10 c"},
.. {"content": "a 1 2 3 4 5 b 6 7 8 9 10 11 c"},
.. {"content": "a 1 2 3 4 5 b 6 7 8 9 10 11 12 c"}
.. ]
.. select NearTokens --match_columns content --query '*N"a b c"'

Note that you must specify ``WITH_POSITION`` to an index column that
is used for near search. If you don't specify ``WITH_POSITION``, near
search can't count distance correctly.

You can customize the max interval of the given tokens (``10`` by
default) by specifying a number after ``*N``. Here is an example that
uses ``2`` as the max interval of the given tokens::

    *N2"..."

Here is an example to customize the max interval of the given tokens:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_search_max_interval.log
.. select NearTokens --match_columns content --query '*N11"a b c"'

To be precious, you can specify a word instead of a token for near
search. Because the passed text is tokenized before near search. A
word consists of one or more tokens. If you specify a word, it may not
work as you expected. For example, ``*N"a1b2c3d"`` matches both ``a 1
b 2 c 3 d`` and ``a b c d 1 2 3``:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_search_word.log
.. load --table NearTokens
.. [
.. {"content": "a 1 b 2 c 3 d"},
.. {"content": "a b c d 1 2 3"}
.. ]
.. select NearTokens --match_columns content --query '*N"a1b2c3d"'

Because ``*N"a1b2c3d"`` equals to ``*N"a 1 b 2 c 3 d"``.

If you want to specify words,
:ref:`query-syntax-near-phrase-search-condition` is what you want.

.. _query-syntax-near-phrase-search-condition:

Near phrase search condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``*NP"phrase1 phrase2 ..."``.

This conditional expression does near phrase search with ``phrase1``,
``phrase2`` and ``...``. Near phrase search searches records that
contain the all specified phrases and there are at most 10 tokens
between them. For example, ``*NP"a1b2c3d"`` matches ``a 1 b 2 c 3 d``
but doesn't match ``a b c d 1 2 3``. Because the latter uses different
order:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_search_search.log
.. select NearTokens --match_columns content --query '*NP"a1b2c3d"'

You can use a phrase that includes spaces by quoting such as
``*NP"\"groonga mroonga\" pgroonga"``. Note that you need to escape
``\"`` in command syntax such as ``*NP"\\\"groonga mroonga\\\"
pgroonga"``. This query matches ``groonga mroonga pgroonga`` but
doesn't match ``groonga pgroonga mroonga`` because ``mroonga`` isn't
right after ``groonga``:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_search_word.log
.. load --table NearTokens
.. [
.. {"content": "groonga mroonga rroonga pgroonga"},
.. {"content": "groonga rroonga pgroonga mroonga"}
.. ]
.. select NearTokens \
..   --match_columns content \
..   --query '*NP"\\\"groonga mroonga\\\" pgroonga"'

You can customize the max interval of the given phrases (``10`` by
default) by specifying a number after ``*NP``. Here is an example that
uses ``2`` as the max interval of the given phrases::

    *NP2"..."

Here is an example to customize the max interval of the given phrases:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_search_max_interval.log
.. select NearTokens --match_columns content --query '*NP1"groonga pgroonga"'

You can use additional interval only for the last phrase. It means
that you can accept more distances only between the second to last
phrase and the last phrase. This is useful for implementing a near
phrase search in the same sentence. If you specify ``.`` (sentence end
phrase) as the last phrase and specify ``-1`` as the additional last
interval, the other specified phrases must be appeared before
``.``. You must append ``$`` to the last phrase like ``.$``.

Here is an example that uses ``-1`` as the additional last interval of
the given phrases::

    *NP10,-1"a b .$"

Here is an example to customize the additional last interval of the
given phrases:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_search_additional_last_interval_negative.log
.. load --table NearTokens
.. [
.. {"content": "x 1 y 2 3 4 . x 1 2 y 3 z 4 5 6 7 ."},
.. {"content": "x 1 2 y 3 4 . x 1 2 y 3 z 4 5 6 7 ."},
.. {"content": "x 1 2 3 y 4 . x 1 y 2 z 3 4 5 6 7 ."},
.. ]
.. select NearTokens --match_columns content --query '*NP2,-1"x y .$"'

You can also use positive number for the additional last interval. If
you specify positive number as the additional last interval, all of
the following conditions must be satisfied:

1. The interval between the first phrase and the second to last
   phrase is less than or equals to ``the max interval``.

2. The interval between the first phrase and the last phrase is less
   than or equals to ``the max interval`` + ``the additional last
   interval``.

If you specify negative number as the additional last interval, the
second condition isn't required. Appearing the last phrase is just
needed.

Here is an example to use positive number as the additional last interval:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_search_additional_last_interval_positive.log
.. select NearTokens --match_columns content --query '*NP2,4"x y .$"'

.. _query-syntax-near-phrase-product-search-condition:

Near phrase product search condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 11.1.1

Its syntax is ``*NPP"(phrase1_1 phrase1_2 ...) (phrase2_1 phrase2_2
...) ..."``.

This conditional expression does multiple
:ref:`query-syntax-near-phrase-search-condition`. Phrases for each
:ref:`query-syntax-near-phrase-search-condition` are computed as
product of ``{phrase1_1, phrase1_2, ...}``, ``{phrase2_1, phrase2_2,
...}`` and ``...``. For example, ``*NPP"(a b c) (d e)"`` uses the
following phrases for near phrase searches:

  * ``a d``
  * ``a e``
  * ``b d``
  * ``b e``
  * ``c d``
  * ``c e``

Here is a simple example:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_product_search_simple.log
.. select NearTokens --match_columns content --query '*NPP"(a x) (b y)"'

You can use the all features of
:ref:`query-syntax-near-phrase-search-condition` such as the max
interval, ``$`` for the last phrase and the additional last
interval.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/near_phrase_product_search_options.log
.. select NearTokens --match_columns content --query '*NPP2,-1"(a x) (b c y) (d$ .$)"'

This is more effective than multiple
:ref:`query-syntax-near-phrase-search-condition` .

.. _query-syntax-ordered-near-phrase-search-condition:

Ordered near phrase search condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 11.0.9

It's syntax is ``*ONP"phrase1 phrase2 ..."``

This conditional expression does ordered near phrase search with
``phrase1``, ``phrase2`` and ``...``. Ordered near phrase search is
similar to :ref:`query-syntax-near-phrase-search-condition` but
ordered near phrase search checks phrases order. For example,
``*ONP"groonga mroonga pgroonga"`` matches ``groonga mroonga rroonga
pgroonga`` but doesn't match ``groonga rroonga pgroonga
mroonga``. Because the latter uses different order:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/ordered_near_phrase_search_search.log
.. select NearTokens \
..   --match_columns content \
..   --query '*ONP"groonga mroonga pgroonga"'

You can use the all features of
:ref:`query-syntax-near-phrase-search-condition` such as the max
interval and the additional last interval. But you don't need to
specify ``$`` for the last phrase because the last phrase in query is
the last phrase.

.. _query-syntax-ordered-near-phrase-product-search-condition:

Ordered near phrase product search condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 11.1.1

Its syntax is ``*ONPP"(phrase1_1 phrase1_2 ...) (phrase2_1 phrase2_2
...) ..."``.

This conditional expression does ordered near phrase product
search. Ordered near phrase product search is similar to
:ref:`query-syntax-near-phrase-product-search-condition` but ordered
near phrase product search checks phrases order like
:ref:`query-syntax-ordered-near-phrase-search-condition`. For example,
``*ONPP"(a b c) (d e)"`` matches ``a 1 d`` but doesn't match ``d 1
a``. Because the latter uses different order.

Here is a simple example:

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/ordered_near_phrase_product_search_simple.log
.. select NearTokens \
..   --match_columns content \
..   --query '*ONPP"(a x) (b y)"'

You can use the all features of
:ref:`query-syntax-near-phrase-search-condition` such as the max
interval and the additional last interval. But you don't need to
specify ``$`` for the last phrase because the last phrase in query is
the last phrase.

.. _query-syntax-similar-search-condition:

Similar search condition
^^^^^^^^^^^^^^^^^^^^^^^^

TODO

.. _query-syntax-equal-condition:

Equal condition
^^^^^^^^^^^^^^^

Its syntax is ``column:value``.

It matches records that ``column`` value is equal to ``value``.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_equal.log
.. select Entries --query _key:Groonga

The expression matches records that ``_key`` column value is
equal to ``Groonga``.

.. _query-syntax-not-equal-condition:

Not equal condition
^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:!value``.

It matches records that ``column`` value isn't equal to ``value``.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_not_equal.log
.. select Entries --query _key:!Groonga

The expression matches records that ``_key`` column value is not equal
to ``Groonga``.

.. _query-syntax-less-than-condition:

Less than condition
^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:<value``.

It matches records that ``column`` value is less than ``value``.

If ``column`` type is numerical type such as ``Int32``, ``column``
value and ``value`` are compared as number. If ``column`` type is text
type such as ``ShortText``, ``column`` value and ``value`` are
compared as bit sequence.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_less_than.log
.. select Entries --query n_likes:<10

The expression matches records that ``n_likes`` column value is less
than ``10``.

.. _query-syntax-greater-than-condition:

Greater than condition
^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:>value``.

It matches records that ``column`` value is greater than ``value``.

If ``column`` type is numerical type such as ``Int32``, ``column``
value and ``value`` are compared as number. If ``column`` type is text
type such as ``ShortText``, ``column`` value and ``value`` are
compared as bit sequence.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_greater_than.log
.. select Entries --query n_likes:>10

The expression matches records that ``n_likes`` column value is greater
than ``10``.

.. _query-syntax-less-than-or-equal-condition:

Less than or equal to condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:<=value``.

It matches records that ``column`` value is less than or equal to
``value``.

If ``column`` type is numerical type such as ``Int32``, ``column``
value and ``value`` are compared as number. If ``column`` type is text
type such as ``ShortText``, ``column`` value and ``value`` are
compared as bit sequence.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_less_than_or_equal_to.log
.. select Entries --query n_likes:<=10

The expression matches records that ``n_likes`` column value is less
than or equal to ``10``.

.. _query-syntax-greater-than-or-equal-condition:

Greater than or equal to condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column:>=value``.

It matches records that ``column`` value is greater than or equal to
``value``.

If ``column`` type is numerical type such as ``Int32``, ``column``
value and ``value`` are compared as number. If ``column`` type is text
type such as ``ShortText``, ``column`` value and ``value`` are
compared as bit sequence.

It doesn't require the default match columns such as ``full text
search condition`` and ``phrase search condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_greater_than_or_equal_to.log
.. select Entries --query n_likes:>=10

The expression matches records that ``n_likes`` column value is
greater than or equal to ``10``.

.. _query-syntax-regular-expression-condition:

Regular expression condition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 5.0.1

Its syntax is ``column:~pattern``.

It matches records that ``column`` value is matched to
``pattern``. ``pattern`` must be valid
:doc:`/reference/regular_expression`.

The following example uses ``.roonga`` as pattern. It matches
``Groonga``, ``Mroonga`` and so on.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_regular_expression.log
.. select Entries --query content:~.roonga

In most cases, regular expression is evaluated sequentially. So it may
be slow against many records.

In some cases, Groonga evaluates regular expression by index. It's
very fast. See :doc:`/reference/regular_expression` for details.

.. _query-syntax-combined-expression:

Combined expression
-------------------

Here is available combined expression list.

.. _query-syntax-logical-or:

Logical OR
^^^^^^^^^^

Its syntax is ``a OR b``.

``a`` and ``b`` are conditional expressions, conbinded expressions or
assignment expressions.

If at least one of ``a`` and ``b`` are matched, ``a OR b`` is matched.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_logical_or.log
.. select Entries --query 'n_likes:>10 OR content:@senna'

The expression matches records that ``n_likes`` column value is
greater than ``10`` or contain a word ``senna`` in ``content`` column
value.

.. _query-syntax-logical-and:

Logical AND
^^^^^^^^^^^

Its syntax is ``a + b`` or just ``a b``.

``a`` and ``b`` are conditional expressions, conbinded expressions or
assignment expressions.

If both ``a`` and ``b`` are matched, ``a + b`` is matched.

You can specify ``+`` the first expression such as ``+a``. The ``+``
is just ignored.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_logical_and.log
.. select Entries --query 'n_likes:>=10 + content:@groonga'

The expression matches records that ``n_likes`` column value is
greater than or equal to ``10`` and contain a word ``groonga`` in
``content`` column value.

.. _query-syntax-logical-and-not:

Logical AND NOT
^^^^^^^^^^^^^^^

Its syntax is ``a - b``.

``a`` and ``b`` are conditional expressions, conbinded expressions or
assignment expressions.

If ``a`` is matched and ``b`` is not matched, ``a - b`` is matched.

You can not specify ``-`` the first expression such as ``-a``. It's
syntax error.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_logical_not.log
.. select Entries --query 'n_likes:>=10 - content:@groonga'

The expression matches records that ``n_likes`` column value is
greater than or equal to ``10`` and don't contain a word ``groonga`` in
``content`` column value.

Grouping
^^^^^^^^

Its syntax is ``(...)``. ``...`` is space separated expression list.

``(...)`` groups one ore more expressions and they can be
processed as an expression. ``a b OR c`` means that ``a`` and ``b``
are matched or ``c`` is matched. ``a (b OR c)`` means that ``a`` and
one of ``b`` and ``c`` are matched.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/query_syntax/simple_grouping.log
.. select Entries --query 'n_likes:<5 content:@senna OR content:@fast'
.. select Entries --query 'n_likes:<5 (content:@senna OR content:@fast)'

The first expression doesn't use grouping. It matches records that
``n_likes:<5`` and ``content:@senna`` are matched or
``content:@fast`` is matched.

The second expression uses grouping. It matches records that
``n_likes:<5`` and one of ``content:@senna`` or ``content:@fast``
are matched.

Assignment expression
---------------------

This section is for advanced users. Because assignment expression is
disabled in ``--query`` option of :doc:`/reference/commands/select` by
default. You need to specify ``ALLOW_COLUMN|ALLOW_UPDATE`` as
``--query_flags`` option value to enable assignment expression.

Assignment expression in query syntax has some limitations. So you
should use :doc:`/reference/grn_expr/script_syntax` instead of query syntax for
assignment.

There is only one syntax for assignment expression. It's ``column:=value``.

``value`` is assigend to ``column``. ``value`` is always processed as
string in query syntax. ``value`` is casted to the type of ``column``
automatically. It causes some limitations. For example, you cannot use
boolean literal such as ``true`` and ``false`` for ``Bool`` type
column. You need to use empty string for ``false`` but query syntax
doesn't support ``column:=`` syntax.

See :doc:`/reference/cast` about cast.
