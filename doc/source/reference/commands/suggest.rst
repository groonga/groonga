.. -*- rst -*-

.. groonga-command
.. % groonga-suggest-create-dataset /tmp/groonga-databases/suggest query
.. database: suggest

``suggest``
===========

.. note::

   The suggest feature specification isn't stable. The
   specification may be changed.

Summary
-------

suggest - returns completion, correction and/or suggestion for a query.

The suggest command returns completion, correction and/or
suggestion for a specified query.

See :doc:`/reference/suggest/introduction` about completion,
correction and suggestion.

Syntax
------
::

 suggest types
         table
         column
         query
         [sortby=-_score]
         [output_columns=_key,_score]
         [offset=0]
         [limit=10]
         [frequency_threshold=100]
         [conditional_probability_threshold=0.2]
         [prefix_search=auto]

Usage
-----

Here are learned data for completion.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-learn-completion.log
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "1", "time": 1312950803.86057, "item": "e"},
.. {"sequence": "1", "time": 1312950803.96857, "item": "en"},
.. {"sequence": "1", "time": 1312950804.26057, "item": "eng"},
.. {"sequence": "1", "time": 1312950804.56057, "item": "engi"},
.. {"sequence": "1", "time": 1312950804.76057, "item": "engin"},
.. {"sequence": "1", "time": 1312950805.86057, "item": "engine", "type": "submit"}
.. ]

Here are learned data for correction.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-learn-correction.log
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "2", "time": 1312950803.86057, "item": "s"},
.. {"sequence": "2", "time": 1312950803.96857, "item": "sa"},
.. {"sequence": "2", "time": 1312950804.26057, "item": "sae"},
.. {"sequence": "2", "time": 1312950804.56057, "item": "saer"},
.. {"sequence": "2", "time": 1312950804.76057, "item": "saerc"},
.. {"sequence": "2", "time": 1312950805.76057, "item": "saerch", "type": "submit"},
.. {"sequence": "2", "time": 1312950809.76057, "item": "serch"},
.. {"sequence": "2", "time": 1312950810.86057, "item": "search", "type": "submit"}
.. ]

Here are learned data for suggestion.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-learn-suggestion.log
.. load --table event_query --each 'suggest_preparer(_id, type, item, sequence, time, pair_query)'
.. [
.. {"sequence": "3", "time": 1312950803.86057, "item": "search engine", "type": "submit"},
.. {"sequence": "3", "time": 1312950808.86057, "item": "web search realtime", "type": "submit"}
.. ]

Here is a completion example.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-completion.log
.. suggest --table item_query --column kana --types complete --frequency_threshold 1 --query en

Here is a correction example.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-correction.log
.. suggest --table item_query --column kana --types correct --frequency_threshold 1 --query saerch

Here is a suggestion example.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-suggestion.log
.. suggest --table item_query --column kana --types suggest --frequency_threshold 1 --query search

Here is a mixed example.

.. groonga-command
.. include:: ../../example/reference/commands/suggest-mixed.log
.. suggest --table item_query --column kana --types complete|correct|suggest --frequency_threshold 1 --query search

Parameters
----------

``types``
  Specifies what types are returned by the suggest
  command.

  Here are available types:

    ``complete``
      The suggest command does completion.

    ``correct``
      The suggest command does correction.

    ``suggest``
      The suggest command does suggestion.

  You can specify one or more types separated by ``|``.
  Here are examples:

    It returns correction::

      correct

    It returns correction and suggestion::

      correct|suggest

    It returns complete, correction and suggestion::

      complete|correct|suggest

``table``
  Specifies table name that has ``item_${DATA_SET_NAME}`` format.
  For example, ``item_query`` is a table name if you created
  dataset by the following command::

    groonga-suggest-create-dataset /tmp/db-path query

``column``
  Specifies a column name that has furigana in
  Katakana in ``table`` table.

``query``
  Specifies query for completion, correction and/or
  suggestion.

``sortby``
  Specifies sort key.

  Default:
    ``-_score``

``output_columns``
  Specifies output columns.

  Default:
    ``_key,_score``

``offset``
  Specifies returned records offset.

  Default:
    ``0``

``limit``
  Specifies number of returned records.

  Default:
    ``10``

``frequency_threshold``
  Specifies threshold for item frequency. Returned records must have
  ``_score`` that is greater than or equal to ``frequency_threshold``.

  Default:
    ``100``

``conditional_probability_threshold``

  Specifies threshold for conditional
  probability. Conditional probability is used for learned
  data. It is probability of query submission when ``query``
  is occurred. Returned records must have conditional
  probability that is greater than or equal to
  ``conditional_probability_threshold``.

  Default:
    ``0.2``

``prefix_search``
  Specifies whether optional prefix search is used or not
  in completion.

  Here are available values:

    ``yes``
      Prefix search is always used.

    ``no``
      Prefix search is never used.

    ``auto``
      Prefix search is used only when other search can't
      find any records.

  Default:
    ``auto``

``similar_search``
  Specifies whether optional similar search is used or not
  in correction.

  Here are available values:

    ``yes``
      Similar search is always used.

    ``no``
      Similar search is never used.

    ``auto``
      Similar search is used only when other search can't
      find any records.

  Default:
    ``auto``

Return value
------------

Here is a returned JSON format::

 {"type1": [["candidate1", score of candidate1],
            ["candidate2", score of candidate2],
            ...],
  "type2": [["candidate1", score of candidate1],
            ["candidate2", score of candidate2],
            ...],
  ...}

``type``

  A type specified by ``types``.

``candidate``

  A candidate for completion, correction or suggestion.

``score of candidate``

  A score of corresponding ``candidate``. It means that
  higher score candidate is more likely candidate for
  completion, correction or suggestion. Returned candidates
  are sorted by ``score of candidate`` descending by
  default.

See also
--------

* :doc:`/reference/suggest`
* :doc:`/reference/executables/groonga-suggest-create-dataset`
