.. -*- rst -*-

Introduction
============

The suggest feature in Groonga provides the following features:

* Completion
* Correction
* Suggestion

Completion
----------

Completion helps user input. If user inputs a partial word,
Groonga can return complete words from registered
words.

For example, there are registered words:

* "groonga"
* "complete"
* "correction"
* "suggest"

An user inputs "co" and groonga returns "complete" and
"correction" because they starts with "co".

An user inputs "sug" and groonga returns "suggest" because
"suggest" starts with "sug".

An user inputs "ab" and groonga returns nothing because no
word starts with "ab".

Correction
----------

Correction also helps user input. If user inputs a wrong
word, groonga can return correct words from registered
correction pairs.

For example, there are registered correction pairs:

+------------+--------------+
| wrong word | correct word |
+============+==============+
| grroonga   | groonga      |
+------------+--------------+
| gronga     | groonga      |
+------------+--------------+
| gronnga    | groonga      |
+------------+--------------+

An user inputs "gronga" and groonga returns "groonga" because
"gronga" is in wrong word and corresponding correct word is
"groonga".

An user inputs "roonga" and groonga returns nothing because
"roonga" isn't in wrong word.

Suggestion
----------

Suggestion helps that user filters many found documents. If
user inputs a query, groonga can return new queries that has
more additional keywords from registered related query
pairs.

For example, there are registered related query pairs:

+----------------------------+--------------------------+
|          keyword           |      related query       |
+============================+==========================+
| groonga                    | groonga search engine    |
+----------------------------+--------------------------+
| search                     | Google search            |
+----------------------------+--------------------------+
| speed                      | groonga speed            |
+----------------------------+--------------------------+

An user inputs "groonga" and groonga returns "groonga search
engine" because "groonga" is in keyword column and related
query column is "groonga search engine".

An user inputs "MySQL" and groonga returns nothing because
"MySQL" isn't in keyword column values.

Learning
--------

The suggest feature requires registered data before using
the feature. Those data can be registered from user inputs.
Gronnga-suggest-httpd and groonga-suggest-learner commands
are provided for the propose.
