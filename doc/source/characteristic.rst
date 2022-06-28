.. -*- rst -*-
.. Groonga project

Characteristics of Groonga
==========================

Groonga overview
----------------

Groonga is a fast and accurate full text search engine based on inverted index. One of the characteristics of Groonga is that a newly registered document instantly appears in search results. Also, Groonga allows updates without read locks. These characteristics result in superior performance on real-time applications.

Groonga is also a column-oriented database management system (DBMS). Compared with well-known row-oriented systems, such as MySQL and PostgreSQL, column-oriented systems are more suited for aggregate queries. Due to this advantage, Groonga can cover weakness of row-oriented systems.

The basic functions of Groonga are provided in a C library. Also, libraries for using Groonga in other languages, such as Ruby, are provided by related projects. In addition, groonga-based storage engines are provided for MySQL and PostgreSQL. These libraries and storage engines allow any application to use Groonga. See `usage examples <https://groonga.org/users/>`_.

Full text search and Instant update
-----------------------------------

In widely used DBMSs, updates are immediately processed, for example, a newly registered record appears in the result of the next query. In contrast, some full text search engines do not support instant updates, because it is difficult to dynamically update inverted indexes, the underlying data structure.

Groonga also uses inverted indexes but supports instant updates. In addition, Groonga allows you to search documents even when updating the document collection. Due to these superior characteristics, Groonga is very flexible as a full text search engine. Also, Groonga always shows good performance because it divides a large task, inverted index merging, into smaller tasks.

Column store and aggregate query
--------------------------------

People can collect more than enough data in the Internet era. However, it is difficult to extract informative knowledge from a large database, and such a task requires a many-sided analysis through trial and error. For example, search refinement by date, time and location may reveal hidden patterns. Aggregate queries are useful to perform this kind of tasks.

An aggregate query groups search results by specified column values and then counts the number of records in each group. For example, an aggregate query in which a location column is specified counts the number of records per location. Making a graph from the result of an aggregate query against a date column is an easy way to visualize changes over time. Also, a combination of refinement by location and an aggregate query against a date column allows visualization of changes over time in specific location. Thus refinement and aggregation are important to perform data mining.

A column-oriented architecture allows Groonga to efficiently process aggregate queries because a column-oriented database, which stores records by column, allows an aggregate query to access only a specified column. On the other hand, an aggregate query on a row-oriented database, which stores records by row, has to access neighbor columns, even though those columns are not required.

Inverted index and tokenizer
----------------------------

An inverted index is a traditional data structure used for large-scale full text search. A search engine based on inverted index extracts index terms from a document when it is added. Then in retrieval, a query is divided into index terms to find documents containing those index terms. In this way, index terms play an important role in full text search and thus the way of extracting index terms is a key to a better search engine.

A tokenizer is a module to extract index terms. A Japanese full text search engine commonly uses a word-based tokenizer (hereafter referred to as a word tokenizer) and/or a character-based n-gram tokenizer (hereafter referred to as an n-gram tokenizer). A word tokenizer-based search engine is superior in time, space and precision, which is the fraction of relevant documents in a search result. On the other hand, an n-gram tokenizer-based search engine is superior in recall, which is the fraction of retrieved documents in the perfect search result. The best choice depends on the application in practice.

Groonga supports both word and n-gram tokenizers. The simplest built-in tokenizer uses spaces as word delimiters. Built-in n-gram tokenizers (n = 1, 2, 3) are also available by default. In addition, a yet another built-in word tokenizer is available if MeCab, a part-of-speech and morphological analyzer, is embedded. Note that a tokenizer is pluggable and you can develop your own tokenizer, such as a tokenizer based on another part-of-speech tagger or a named-entity recognizer.

Sharable storage and read lock-free
-----------------------------------

Multi-core processors are mainstream today and the number of cores per processor is increasing. In order to exploit multiple cores, executing multiple queries in parallel or dividing a query into sub-queries for parallel processing is becoming more important.

A database of Groonga can be shared with multiple threads/processes. Also, multiple threads/processes can execute read queries in parallel even when another thread/process is executing an update query because Groonga uses read lock-free data structures. This feature is suited to a real-time application that needs to update a database while executing read queries. In addition, Groonga allows you to build flexible systems. For example, a database can receive read queries through the built-in HTTP server of Groonga while accepting update queries through MySQL.

Geo-location (latitude and longitude) search
--------------------------------------------

Location services are getting more convenient because of mobile devices with GPS. For example, if you are going to have lunch or dinner at a nearby restaurant, a local search service for restaurants may be very useful, and for such services, fast geo-location search is becoming more important.

Groonga provides inverted index-based fast geo-location search, which supports a query to find points in a rectangle or circle. Groonga gives high priority to points near the center of an area. Also, Groonga supports distance measurement and you can sort points by distance from any point.

Groonga library
---------------

The basic functions of Groonga are provided in a C library and any application can use Groonga as a full text search engine or a column-oriented database. Also, libraries for languages other than C/C++, such as Ruby, are provided in related projects. See `related projects <https://groonga.org/related-projects.html>`_ for details.

Groonga server
--------------

Groonga provides a built-in server command which supports HTTP, the memcached binary protocol and the Groonga Query Transfer Protocol (:doc:`/spec/gqtp`). Also, a Groonga server supports query caching, which significantly reduces response time for repeated read queries. Using this command, Groonga is available even on a server that does not allow you to install new libraries.

Mroonga storage engine
----------------------

Groonga works not only as an independent column-oriented DBMS but also as storage engines of well-known DBMSs. For example, `Mroonga <https://mroonga.org/>`_ is a MySQL pluggable storage engine using Groonga. By using Mroonga, you can use Groonga for column-oriented storage and full text search. A combination of a built-in storage engine, MyISAM or InnoDB, and a Groonga-based full text search engine is also available. All the combinations have good and bad points and the best one depends on the application. See `related projects <https://groonga.org/related-projects.html>`_ for details.
