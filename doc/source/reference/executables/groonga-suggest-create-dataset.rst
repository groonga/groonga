.. -*- rst -*-

groonga-suggest-create-dataset
==============================

NAME
----

groonga-suggest-create-dataset - Defines schema for a suggestion dataset

SYNOPSTIS
---------

::

 groonga-suggest-create-dataset [options] DATABASE DATASET

DESCTIPION
----------

groonga-suggest-create-dataset creates a dataset for :doc:`/reference/suggest`. A database has many datasets. This command just defines schema for a suggestion dataset.

This command generates some tables and columns for :doc:`/reference/suggest`.

Here is the list of such tables. If you specify 'query' as dataset name, following '_DATASET' suffix are replaced. Thus, 'item_query', 'pair_query', 'sequence_query', 'event_query' tables are generated.

* event_type
* bigram
* kana
* item_DATASET
* pair_DATASET
* sequence_DATASET
* event_DATASET
* configuration

OPTIONS
-------

None.

EXIT STATUS
-----------

TODO

FILES
-----

TODO

EXAMPLE
-------

TODO

SEE ALSO
--------

:doc:`/reference/suggest`
:doc:`groonga-suggest-httpd`
:doc:`groonga-suggest-learner`
