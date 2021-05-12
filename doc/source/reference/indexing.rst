.. -*- rst -*-

.. groonga-command
.. database: indexing

Indexing
========

Groonga supports both online index construction and offline
index construction since 2.0.0.

.. _online-index-construction:

Online index construction
-------------------------

In online index construction, registered documents can be
searchable quickly while indexing. But indexing requires
more cost rather than indexing by offline index
construction.

Online index construction is suitable for a search system
that values freshness. For example, a search system for
tweets, news, blog posts and so on will value
freshness. Online index construction can make fresh
documents searchable and keep searchable while indexing.

.. _offline-index-construction:

Offline index construction
--------------------------

In offline index construction, indexing cost is less than
indexing cost by online index construction. Indexing time
will be shorter. Index will be smaller. Resources required
for indexing will be smaller. But a registering document
cannot be searchable until all registered documents are
indexed.

Offline index construction is suitable for a search system
that values less required resources. If a search system
doesn't value freshness, offline index construction will be
suitable. For example, a reference manual search system
doesn't value freshness because a reference manual will be
updated only at a release.

How to use
----------

Groonga uses online index construction by default. We
register a document, we can search it quickly.

Groonga uses offline index construction by adding an index
to a column that already has data.

We define a schema:

.. groonga-command
.. include:: ../example/reference/indexing-schema.log
.. table_create Tweets TABLE_NO_KEY
.. column_create Tweets content COLUMN_SCALAR ShortText
.. table_create Lexicon TABLE_HASH_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto

We register data:

.. groonga-command
.. include:: ../example/reference/indexing-data.log
.. load --table Tweets
.. [
.. {"content":"Hello!"},
.. {"content":"I just start it!"},
.. {"content":"I'm sleepy... Have a nice day... Good night..."}
.. ]

We can search with sequential search when we don't have index:

.. groonga-command
.. include:: ../example/reference/indexing-search-without-index.log
.. select Tweets --match_columns content --query 'good nice'

We create index for ``Tweets.content``. Already registered
data in ``Tweets.content`` are indexed by offline index
construction:

.. groonga-command
.. include:: ../example/reference/indexing-offline-index-construction.log
.. column_create Lexicon tweet COLUMN_INDEX|WITH_POSITION Tweets content

We search with index. We get a matched record:

.. groonga-command
.. include:: ../example/reference/indexing-search-after-offline-index-construction.log
.. select Tweets --match_columns content --query 'good nice'

We register data again. They are indexed by online index
construction:

.. groonga-command
.. include:: ../example/reference/indexing-online-index-construction.log
.. load --table Tweets
.. [
.. {"content":"Good morning! Nice day."},
.. {"content":"Let's go shopping."}
.. ]

We can also get newly registered records by searching:

.. groonga-command
.. include:: ../example/reference/indexing-search-after-online-index-construction.log
.. select Tweets --match_columns content --query 'good nice'
