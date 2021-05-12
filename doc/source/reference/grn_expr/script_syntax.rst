.. -*- rst -*-

.. groonga-command
.. database: reference_grn_expr_script_syntax

Script syntax
=============

Script syntax is a syntax to specify complex search condition. It is
similar to ECMAScript. For example, ``_key == "book"`` means that
groonga searches records that ``_key`` value is ``"book"``. All values
are string in :doc:`query_syntax` but its own type in script
syntax. For example, ``"book"`` is string, ``1`` is integer,
``TokenBigram`` is the object whose name is ``TokenBigram`` and so on.

Script syntax doesn't support full ECMAScript syntax. For example,
script syntax doesn't support statement such as ``if`` control
statement, ``for`` iteration statement and variable definition
statement. Function definion is not supported too. But script syntax
addes the original additional operators.  They are described after
ECMAScript syntax is described.

.. _script-syntax-security:

Security
--------

For security reason, you should not pass an input from users to
Groonga directly. If there is an evil user, the user may input a query
that retrieves records that should not be shown to the user.

Think about the following case.

A Groonga application constructs a Groonga request by the following
program::

  filter = "column @ \"#{user_input}\""
  select_options = {
    # ...
    :filter => filter,
  }
  groonga_client.select(select_options)

``user_input`` is an input from user. If the input is ``query``,
here is the constructed :ref:`select-filter` parameter::

  column @ "query"

If the input is ``x" || true || "``, here is the constructed
:ref:`select-filter` parameter::

  column @ "x" || true || ""

This query matches to all records. The user will get all records from
your database. The user may be evil.

It's better that you just receive an user input as a value. It means
that you don't accept that user input can contain operator such as
``@`` and ``&&``. If you accept operator, user can create evil query.

If user input has only value, you blocks evil query by escaping user
input value. Here is a list how to escape user input value:

  * True value: Convert it to ``true``.
  * False value: Convert it to ``false``.
  * Numerical value: Convert it to :ref:`script-syntax-literal-integer`
    or :ref:`script-syntax-literal-float`. For example, ``1.2``,
    ``-10``, ``314e-2`` and so on.
  * String value: Replace ``"`` with ``\"`` and ``\`` with ``\\`` in
    the string value and surround substituted string value by
    ``"``. For example, ``double " quote and back \ slash`` should be
    converted to ``"double \" quote and back \\ slash"``.

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

.. _script-syntax-literals:

Literals
--------

.. _script-syntax-literal-integer:

Integer
^^^^^^^

Integer literal is sequence of ``0`` to ``9`` such as
``1234567890``. ``+`` or ``-`` can be prepended as sign such as
``+29`` and ``-29``. Integer literal must be decimal. Octal notation,
hex and so on can't be used.

The maximum value of integer literal is ``9223372036854775807`` (``= 2
** 63 - 1``). The minimum value of integer literal is
``-9223372036854775808`` (``= -(2 ** 63)``).

.. _script-syntax-literal-float:

Float
^^^^^

Float literal is sequence of ``0`` to ``9``, ``.`` and ``0`` to ``9``
such as ``3.14``. ``+`` or ``-`` can be prepended as sign such as
``+3.14`` and ``-3.14``. ``${RADIX}e${EXPORNENTIAL}`` and
``${RADIX}E${EXPORNENTIAL}`` formats are also supported. For example,
``314e-2`` is the same as ``3.14``.

.. _script-syntax-literal-string:

String
^^^^^^

String literal is ``"..."``. You need to escape ``"`` in literal by
prepending ``\\'' such as ``\"``. For example, ``"Say \"Hello!\"."`` is
a literal for ``Say "Hello!".`` string.

String encoding must be the same as encoding of database. The default
encoding is UTF-8. It can be changed by ``--with-default-encoding``
configure option, ``--encodiong`` :doc:`/reference/executables/groonga` option
and so on.

.. _script-syntax-literal-boolean:

Boolean
^^^^^^^

Boolean literal is ``true`` and ``false``. ``true`` means true and
``false`` means false.

.. _script-syntax-literal-null:

Null
^^^^

Null literal is ``null``. Groonga doesn't support null value but null
literal is supported.

.. _script-syntax-literal-time:

Time
^^^^

.. note::

   This is the groonga original notation.

Time literal doesn't exit. There are string time notation, integer
time notation and float time notation.

String time notation is ``"YYYY/MM/DD hh:mm:ss.uuuuuu"`` or
``"YYYY-MM-DD hh:mm:ss.uuuuuu"``. ``YYYY`` is year, ``MM`` is month,
``DD`` is day, ``hh`` is hour, ``mm`` is minute, ``ss`` is second and
``uuuuuu`` is micro second. It is local time. For example,
``"2012/07/23 02:41:10.436218"`` is ``2012-07-23T02:41:10.436218`` in
ISO 8601 format.

Integer time notation is the number of seconds that have elapsed since
midnight UTC, January 1, 1970. It is also known as POSIX time. For
example, ``1343011270`` is ``2012-07-23T02:41:10Z`` in ISO 8601 format.

Float time notation is the number of seconds and micro seconds that
have elapsed since midnight UTC, January 1, 1970. For example,
``1343011270.436218`` is ``2012-07-23T02:41:10.436218Z`` in ISO 8601
format.

.. _script-syntax-literal-geo-point:

Geo point
^^^^^^^^^

.. note::

   This is the groonga original notation.

Geo point literal doesn't exist. There is string geo point notation.

String geo point notation has the following patterns:

  * ``"LATITUDE_IN_MSECxLONGITUDE_IN_MSEC"``
  * ``"LATITUDE_IN_MSEC,LONGITUDE_IN_MSEC"``
  * ``"LATITUDE_IN_DEGREExLONGITUDE_IN_DEGREE"``
  * ``"LATITUDE_IN_DEGREE,LONGITUDE_IN_DEGREE"``

``x`` and ``,`` can be used for separator. Latitude and longitude can
be represented in milliseconds or degree.

.. _script-syntax-literal-array:

Array
^^^^^

Array literal is ``[element1, element2, ...]``.

.. _script-syntax-literal-object:

Object literal
^^^^^^^^^^^^^^

Object literal is ``{name1: value1, name2: value2, ...}``. Groonga
doesn't support object literal yet.

Control syntaxes
----------------

Script syntax doesn't support statement. So you cannot use control
statement such as ``if``. You can only use ``A ? B : C`` expression as
control syntax.

``A ? B : C`` returns ``B`` if ``A`` is true, ``C`` otherwise.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_control_syntax_ternary_operator.log
.. select Entries --filter 'n_likes == (_id == 1 ? 5 : 3)'

The expression matches records that ``_id`` column value is equal to ``1``
and ``n_likes`` column value is equal to ``5`` or ``_id`` column value is
not equal to 1 and ``n_likes`` column value is equal to ``3``.

Grouping
--------

Its syntax is ``(...)``. ``...`` is comma separated expression list.

``(...)`` groups one ore more expressions and they can be processed as
an expression. ``a && b || c`` means that ``a`` and ``b`` are matched
or ``c`` is matched. ``a && (b || c)`` means that ``a`` and one of
``b`` and ``c`` are matched.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_grouping.log
.. select Entries --filter 'n_likes < 5 && content @ "senna" || content @ "fast"'
.. select Entries --filter 'n_likes < 5 && (content @ "senna" || content @ "fast")'

The first expression doesn't use grouping. It matches records that
``n_likes < 5`` and ``content @ "senna"`` are matched or
``content @ "fast"`` is matched.

The second expression uses grouping. It matches records that ``n_likes
< 5`` and one of ``content @ "senna"`` or ``content @ "fast"`` are
matched.

Function call
-------------

Its syntax is ``name(arugment1, argument2, ...)``.

``name(argument1, argument2, ...)`` calls a function that is named
``name`` with arguments ``argument1``, ``argument2`` and ``...``.

See :doc:`/reference/function` for available functin list.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_function.log
.. select Entries --filter 'edit_distance(_key, "Groonga") <= 1'

The expression uses :doc:`/reference/functions/edit_distance`. It
matches records that ``_key`` column value is similar to
``"Groonga"``. Similality of ``"Groonga"`` is computed as edit
distance. If edit distance is less than or equal to 1, the value is
treated as similar. In this case, ``"Groonga"`` and ``"Mroonga"`` are
treated as similar.

Basic operators
---------------

Groonga supports operators defined in ECMAScript.

Arithmetic operators
^^^^^^^^^^^^^^^^^^^^

Here are arithmetic operators.

Addition operator
"""""""""""""""""

Its syntax is ``number1 + number2``.

The operator adds ``number1`` and ``number2`` and returns the result.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_addition_operator.log
.. select Entries --filter 'n_likes == 10 + 5'

The expression matches records that ``n_likes`` column value is equal
to ``15`` (= ``10 + 5``).

Subtraction operator
""""""""""""""""""""

Its syntax is ``number1 - number2``.

The operator subtracts ``number2`` from ``number1`` and returns the result.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_subtraction_operator.log
.. select Entries --filter 'n_likes == 20 - 5'

The expression matches records that ``n_likes`` column value is equal
to ``15`` (= ``20 - 5``).

Multiplication operator
"""""""""""""""""""""""

Its syntax is ``number1 * number2``.

The operator multiplies ``number1`` and ``number2`` and returns the result.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_multiplication_operator.log
.. select Entries --filter 'n_likes == 3 * 5'

The expression matches records that ``n_likes`` column value is equal
to ``15`` (= ``3 * 5``).

Division operator
"""""""""""""""""

Its syntax is ``number1 / number2`` and ``number1 % number2``.

The operator divides ``number2`` by ``number1``. ``/`` returns the
quotient of result. ``%`` returns the remainder of result.

Here is simple examples.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_division_operator_quotient.log
.. select Entries --filter 'n_likes == 26 / 7'

The expression matches records that ``n_likes`` column value is equal
to ``3`` (= ``26 / 7``).

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_division_operator_remainder.log
.. select Entries --filter 'n_likes == 26 % 7'

The expression matches records that ``n_likes`` column value is equal
to ``5`` (= ``26 % 7``).

Logical operators
^^^^^^^^^^^^^^^^^

Here are logical operators.

Logical NOT operator
""""""""""""""""""""

Its syntax is ``!condition``.

The operator inverts boolean value of ``condition``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_logical_not_operator.log
.. select Entries --filter '!(n_likes == 5)'

The expression matches records that ``n_likes`` column value is not
equal to ``5``.

Logical AND operator
""""""""""""""""""""

Its syntax is ``condition1 && condition2``.

The operator returns true if both of ``condition1`` and
``condition2`` are true, false otherwise.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_logical_and_operator.log
.. select Entries --filter 'content @ "fast" && n_likes >= 10'

The expression matches records that ``content`` column value has the
word ``fast`` and ``n_likes`` column value is greater or equal to
``10``.

Logical OR operator
"""""""""""""""""""

Its syntax is ``condition1 || condition2``.

The operator returns true if either ``condition1`` or ``condition2`` is
true, false otherwise.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_logical_or_operator.log
.. select Entries --filter 'n_likes == 5 || n_likes == 10'

The expression matches records that ``n_likes`` column value is equal
to ``5`` or ``10``.

Logical AND NOT operator
""""""""""""""""""""""""

Its syntax is ``condition1 &! condition2``.

The operator returns true if ``condition1`` is true but ``condition2``
is false, false otherwise. It returns difference set.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_logical_but_operator.log
.. select Entries --filter 'content @ "fast" &! content @ "mroonga"'

The expression matches records that ``content`` column value has the
word ``fast`` but doesn't have the word ``mroonga``.

.. _script-syntax-bitwise-operators:

Bitwise operators
^^^^^^^^^^^^^^^^^

Here are bitwise operators.

.. _script-syntax-bitwise-not:

Bitwise NOT operator
""""""""""""""""""""

Its syntax is ``~number``.

The operator returns bitwise NOT of ``number``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_bitwise_not_operator.log
.. select Entries --filter '~n_likes == -6'

The expression matches records that ``n_likes`` column value is equal
to ``5`` because bitwise NOT of ``5`` is equal to ``-6``.

.. _script-syntax-bitwise-and:

Bitwise AND operator
""""""""""""""""""""

Its syntax is ``number1 & number2``.

The operator returns bitwise AND between ``number1`` and ``number2``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_bitwise_and_operator.log
.. select Entries --filter '(n_likes & 1) == 1'

The expression matches records that ``n_likes`` column value is even
number because bitwise AND between an even number and ``1`` is equal
to ``1`` and bitwise AND between an odd number and ``1`` is equal to
``0``.

.. _script-syntax-bitwise-or:

Bitwise OR operator
^^^^^^^^^^^^^^^^^^^

Its syntax is ``number1 | number2``.

The operator returns bitwise OR between ``number1`` and ``number2``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_bitwise_or_operator.log
.. select Entries --filter 'n_likes == (1 | 4)'

The expression matches records that ``n_likes`` column value is equal
to ``5`` (= ``1 | 4``).

.. _script-syntax-bitwise-xor:

Bitwise XOR operator
^^^^^^^^^^^^^^^^^^^^

Its syntax is ``number1 ^ number2``.

The operator returns bitwise XOR between ``number1`` and ``number2``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_bitwise_xor_operator.log
.. select Entries --filter 'n_likes == (10 ^ 15)'

The expression matches records that ``n_likes`` column value is equal
to ``5`` (= ``10 ^ 15``).

.. _script-syntax-shift-operators:

Shift operators
^^^^^^^^^^^^^^^

Here are shift operators.

.. _script-syntax-shift-left:

Left shift operator
"""""""""""""""""""

Its syntax is ``number1 << number2``.

The operator performs a bitwise left shift operation on ``number1`` by
``number2``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_left_shift_operator.log
.. select Entries --filter 'n_likes == (5 << 1)'

The expression matches records that ``n_likes`` column value is equal
to ``10`` (= ``5 << 1``).

.. _script-syntax-shift-signed-right:

Signed right shift operator
"""""""""""""""""""""""""""

Its syntax is ``number1 >> number2``.

The operator shifts bits of ``number1`` to right by ``number2``. The sign
of the result is the same as ``number1``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_signed_right_shift_operator.log
.. select Entries --filter 'n_likes == -(-10 >> 1)'

The expression matches records that ``n_likes`` column value is equal
to ``5`` (= ``-(-10 >> 1)`` = ``-(-5)``).

.. _script-syntax-shift-unsigned-right:

Unsigned right shift operator
"""""""""""""""""""""""""""""

Its syntax is ``number1 >>> number2``.

The operator shifts bits of ``number1`` to right by ``number2``. The
leftmost ``number2`` bits are filled by ``0``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_unsigned_right_shift_operator.log
.. select Entries --filter 'n_likes == (2147483648 - (-10 >>> 1))'

The expression matches records that ``n_likes`` column value is equal
to ``5`` (= ``2147483648 - (-10 >>> 1)`` = ``2147483648 - 2147483643``).

Comparison operators
^^^^^^^^^^^^^^^^^^^^

Here are comparison operators.

.. _script-syntax-equal-operator:

Equal operator
""""""""""""""

Its syntax is ``object1 == object2``.

The operator returns true if ``object1`` equals to ``object2``, false
otherwise.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_equal_operator.log
.. select Entries --filter 'n_likes == 5'

The expression matches records that ``n_likes`` column value is equal
to ``5``.

Not equal operator
""""""""""""""""""

Its syntax is ``object1 != object2``.

The operator returns true if ``object1`` does not equal to
``object2``, false otherwise.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_not_equal_operator.log
.. select Entries --filter 'n_likes != 5'

The expression matches records that ``n_likes`` column value is not
equal to ``5``.

Less than operator
""""""""""""""""""

TODO: ...

Less than or equal to operator
""""""""""""""""""""""""""""""

TODO: ...

Greater than operator
"""""""""""""""""""""

TODO: ...

Greater than or equal to operator
"""""""""""""""""""""""""""""""""

TODO: ...

Assignment operators
--------------------


Addition assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 += column2``.

The operator performs addition assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_addition_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score += n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs addition assignment operation such as '_score = _score + n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 + 3`` is evaluated and stored to ``_score`` column as the execution result.

Subtraction assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 -= column2``.

The operator performs subtraction assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_subtraction_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score -= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score - n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 - 3`` is evaluated and stored to ``_score`` column as the execution result.

Multiplication assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 *= column2``.

The operator performs multiplication assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_multiplication_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score *= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score * n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 * 3`` is evaluated and stored to ``_score`` column as the execution result.

Division assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 /= column2``.

The operator performs division assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_division_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score /= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score / n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 / 3`` is evaluated and stored to ``_score`` column as the execution result.

Modulo assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 %= column2``.

The operator performs modulo assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_modulo_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score %= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score % n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 % 3`` is evaluated and stored to ``_score`` column as the execution result.

Bitwise left shift assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 <<= column2``.

The operator performs left shift assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_left_shift_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score <<= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score << n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 << 3`` is evaluated and stored to ``_score`` column as the execution result.

Bitwise signed right shift assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column2 >>= column2``.

The operator performs signed right shift assignment operation on column1 by column2.

Bitwise unsigned right shift assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 >>>= column2``.

The operator performs unsigned right shift assignment operation on column1 by column2.

Bitwise AND assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 &= column2``.

The operator performs bitwise AND assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_and_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score &= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score & n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Groonga" as the ``_key``
is 10.

So the expression ``1 & 10`` is evaluated and stored to ``_score`` column as the execution result.

.. _script-syntax-bitwise-or-assign:

Bitwise OR assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 |= column2``.

The operator performs bitwise OR assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_or_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score |= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score | n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Groonga" as the ``_key``
is 10.

So the expression ``1 | 10`` is evaluated and stored to ``_score`` column as the execution result.

Bitwise XOR assignment operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column1 ^= column2``.

The operator performs bitwise XOR assignment operation on column1 by column2.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_xor_assignment_operator.log
.. select Entries --output_columns _key,n_likes,_score --filter true --scorer '_score ^= n_likes'

The value of ``_score`` by ``--filter`` is always 1 in this case,
then performs subtraction assignment operation such as '_score = _score ^ n_likes' for each records.

For example, the value of ``_score`` about the record which stores "Good-bye Senna" as the ``_key``
is 3.

So the expression ``1 ^ 3`` is evaluated and stored to ``_score`` column as the execution result.

Original operators
------------------

Script syntax adds the original binary opearators to ECMAScript
syntax. They operate search specific operations. They are starts with
``@`` or ``*``.

.. _script-syntax-match-operator:

Match operator
^^^^^^^^^^^^^^

Its syntax is ``column @ value``.

The operator searches ``value`` by inverted index of ``column``.
Normally, full text search is operated but tag search can be operated.
Because tag search is also implemented by inverted index.

:doc:`query_syntax` uses this operator by default.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_match_operator.log
.. select Entries --filter 'content @ "fast"' --output_columns content

The expression matches records that contain a word ``fast`` in
``content`` column value.

.. _script-syntax-prefix-search-operator:

Prefix search operator
^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column @^ value``.

The operator does prefix search with ``value``. Prefix search searches
records that contain a word that starts with ``value``.

You can use fast prefix search against a column. The column must be
indexed and index table must be patricia trie table
(``TABLE_PAT_KEY``) or double array trie table
(``TABLE_DAT_KEY``). You can also use fast prefix search against
``_key`` pseudo column of patricia trie table or double array trie
table. You don't need to index ``_key``.

Prefix search can be used with other table types but it causes all
records scan. It's not problem for small records but it spends more
time for large records.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_prefix_search_operator.log
.. select Entries --filter '_key @^ "Goo"' --output_columns _key

The expression matches records that contain a word that starts with
``Goo`` in ``_key`` pseudo column value. ``Good-bye Senna`` and
``Good-bye Tritonn`` are matched with the expression.

.. _script-syntax-suffix-search-operator:

Suffix search operator
^^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``column @$ value``.

This operator does suffix search with ``value``. Suffix search
searches records that contain a word that ends with ``value``.

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

Here is a simple example. It uses fast suffix search for hiragana in
Japanese that is one of non-ASCII characters.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_suffix_search_operator.log
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

.. _script-syntax-near-search-operator:

Near search operator
^^^^^^^^^^^^^^^^^^^^

Its syntax is one of them::

  column *N "word1 word2 ..."
  column *N${MAX_INTERVAL} "word1 word2 ..."

Here are the examples of the second form::

  column *N29 "word1 word2 ..."
  column *N-1 "word1 word2 ..."

The first example means that ``29`` is used for the max interval.

The second example means that ``-1`` is used for the max interval.
``-1`` max interval means no limit.

The operator does near search with words ``word1 word2 ...``. Near
search searches records that contain the words and the words are
appeared in the specified order and the max interval.

The max interval is ``10`` by default. The unit of the max interval is
the number of characters in N-gram family tokenizers and the number of
words in morphological analysis family tokenizers.

However, ``TokenBigram`` doesn't split ASCII only word into tokens.
Because ``TokenBigram`` uses white-space-separate like tokenize method
for ASCII characters in this case.

So the unit for ASCII words with ``TokenBigram`` is the number of
words even if ``TokenBigram`` is a N-gram family tokenizer.

Note that an index column for full text search must be defined for
``column``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_near_search_operator.log
.. select Entries --filter 'content *N "I fast"'      --output_columns content
.. select Entries --filter 'content *N "I Really"'    --output_columns content
.. select Entries --filter 'content *N "also Really"' --output_columns content

The first expression matches records that contain ``I`` and ``fast``
and the max interval of those words are in 10 words. So the record
that its content is ``I also started to use mroonga. It's also very
fast! ...`` is matched. The number of words between ``I`` and ``fast``
is just 10.

The second expression matches records that contain ``I`` and
``Really`` and the max interval of those words are in 10 words. So the
record that its content is ``I also started to use mroonga. It's also
very fast! Really fast!`` is not matched. The number of words between
``I`` and ``Really`` is 11.

The third expression matches records that contain ``also`` and
``Really`` and the max interval of those words are in 10 words. So
the record that its content is ``I also st arted to use mroonga. It's
also very fast! Really fast!`` is matched. The number of words between
``also`` and ``Really`` is 10.

.. _script-syntax-near-phrase-search-operator:

Near phrase search operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Its syntax is one of them::

  column *NP "phrase1 phrase2 ..."
  column *NP${MAX_INTERVAL} "phrase1 phrase2 ..."

Here are the examples of the second form::

  column *NP29 "phrase1 phrase2 ..."
  column *NP-1 "phrase1 phrase2 ..."

The first example means that ``29`` is used for the max interval.

The second example means that ``-1`` is used for the max interval.
``-1`` max interval means no limit.

The operator does near phrase search with phrases ``phrase1 phrase2
...``. Near phrase search searches records that contain the phrases
and the phrases are appeared in the specified order and the max
interval.

The max interval is ``10`` by default. The unit of the max interval is
the number of characters in N-gram family tokenizers and the number of
words in morphological analysis family tokenizers.

However, ``TokenBigram`` doesn't split ASCII only word into tokens.
Because ``TokenBigram`` uses white-space-separate like tokenize method
for ASCII characters in this case.

So the unit for ASCII words with ``TokenBigram`` is the number of
words even if ``TokenBigram`` is a N-gram family tokenizer.

Note that an index column for full text search must be defined for
``column``.

TODO: Use index that has ``TokenNgram("unify_alphabet", false)``
tokenizer to show difference with near search with English text.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_near_phrase_search_operator.log
.. select Entries --filter 'content *NP "I fast"'      --output_columns content
.. select Entries --filter 'content *NP "I Really"'    --output_columns content
.. select Entries --filter 'content *NP "also Really"' --output_columns content

The first expression matches records that contain ``I`` and ``fast``
and the max interval of those words are in 10 words. So the record
that its content is ``I also started to use mroonga. It's also very
fast! ...`` is matched. The number of words between ``I`` and ``fast``
is just 10.

The second expression matches records that contain ``I`` and
``Really`` and the max interval of those words are in 10 words. So the
record that its content is ``I also started to use mroonga. It's also
very fast! Really fast!`` is not matched. The number of words between
``I`` and ``Really`` is 11.

The third expression matches records that contain ``also`` and
``Really`` and the max interval of those words are in 10 words. So
the record that its content is ``I also st arted to use mroonga. It's
also very fast! Really fast!`` is matched. The number of words between
``also`` and ``Really`` is 10.

.. _script-syntax-similar-search-operator:

Similar search
^^^^^^^^^^^^^^

Its syntax is ``column *S "document"``.

The operator does similar search with document ``document``. Similar
search searches records that have similar content to
``document``.

Note that an index column for full text search must be defined for
``column``.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_similar_search_operator.log
.. select Entries --filter 'content *S "I migrated all Solr system!"' --output_columns content

The expression matches records that have similar content to ``I
migrated all Solr system!``. In this case, records that have ``I
migrated all XXX system!`` content are matched.

You should use ``TokenMecab`` tokenizer for similar search against Japanese documents.
Because ``TokenMecab`` will tokenize target documents to almost words, it improves similar search precision.

.. _script-syntax-term-extract-operator:

Term extract operator
^^^^^^^^^^^^^^^^^^^^^

Its syntax is ``_key *T "document"``.

The operator extracts terms from ``document``. Terms must be
registered as keys of the table of ``_key``.

Note that the table must be patricia trie (``TABLE_PAT_KEY``) or
double array trie (``TABLE_DAT_KEY``). You can't use hash table
(``TABLE_HASH_KEY``) and array (``TABLE_NO_KEY``) because they don't
support longest common prefix search. Longest common prefix search is
used to implement the operator.

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_term_extract_operator.log
.. table_create Words TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
.. load --table Words
.. [
.. {"_key": "groonga"},
.. {"_key": "mroonga"},
.. {"_key": "Senna"},
.. {"_key": "Tritonn"}
.. ]
.. select Words --filter '_key *T "Groonga is the successor project to Senna."' --output_columns _key

The expression extrcts terms that included in document ``Groonga is
the successor project to Senna.``. In this case, ``NormalizerAuto``
normalizer is specified to ``Words``. So ``Groonga`` can be extracted
even if it is loaded as ``groonga`` into ``Words``. All of extracted
terms are also normalized.

.. _script-syntax-regular-expression-operator:

Regular expression operator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 5.0.1

Its syntax is ``column @~ "pattern"``.

The operator searches records by the regular expression
``pattern``. If a record's ``column`` value is matched to ``pattern``,
the record is matched.

``pattern`` must be valid regular expression syntax. See
:doc:`/reference/regular_expression` about regular expression syntax
details.

The following example uses ``.roonga`` as pattern. It matches
``Groonga``, ``Mroonga`` and so on.

.. groonga-command
.. include:: ../../example/reference/grn_expr/script_syntax/simple_regular_expression_operator.log
.. select Entries --filter 'content @~ ".roonga"'

In most cases, regular expression is evaluated sequentially. So it may
be slow against many records.

In some cases, Groonga evaluates regular expression by index. It's
very fast. See :doc:`/reference/regular_expression` for details.
