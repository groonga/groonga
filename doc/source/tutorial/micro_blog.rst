.. -*- rst -*-

.. groonga-command
.. database: tutorial-micro-blog

Let's create micro-blog
=======================

Let's create micro-blog with full text search by Groonga.
Micro-blog is one of the broadcast medium in the forms of blog. It is mainly used to post small messages like a Twitter.

Create a table
--------------

Let's create table.

::

 table_create --name Users --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Comments --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name HashTags --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Bigram --flags TABLE_PAT_KEY --key_type ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
 table_create --name GeoIndex --flags TABLE_PAT_KEY --key_type WGS84GeoPoint

 column_create --table Users --name name --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name follower --flags COLUMN_VECTOR --type Users
 column_create --table Users --name favorites --flags COLUMN_VECTOR --type Comments
 column_create --table Users --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Users --name location_str --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name description --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name followee --flags COLUMN_INDEX --type Users --source follower

 column_create --table Comments --name comment --flags COLUMN_SCALAR --type ShortText
 column_create --table Comments --name last_modified --flags COLUMN_SCALAR --type Time
 column_create --table Comments --name replied_to --flags COLUMN_SCALAR --type Comments
 column_create --table Comments --name replied_users --flags COLUMN_VECTOR --type Users
 column_create --table Comments --name hash_tags --flags COLUMN_VECTOR --type HashTags
 column_create --table Comments --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Comments --name posted_by --flags COLUMN_SCALAR --type Users
 column_create --table Comments --name favorited_by --flags COLUMN_INDEX --type Users --source favorites

 column_create --table HashTags --name hash_index --flags COLUMN_INDEX --type Comments --source hash_tags

 column_create --table Bigram --name users_index --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Users --source name,location_str,description
 column_create --table Bigram --name comment_index --flags COLUMN_INDEX|WITH_POSITION --type Comments --source comment

 column_create --table GeoIndex --name users_location --type Users --flags COLUMN_INDEX --source location
 column_create --table GeoIndex --name comments_location --type Comments --flags COLUMN_INDEX --source location

Users table
^^^^^^^^^^^

This is the table which stores user information.
It stores name of user, profile, list of follower and so on.

``_key``
  User ID

``name``
  User name

``follower``
  List of following users

``favorites``
  List of favorite comments

``location``
  Current location of user (geolocation)

``location_str``
  Current location of user (string)

``description``
  User profile

``followee``
  Indexes for ``follower`` column in ``Users`` table.
  With this indexes, you can search users who follows the person.

Comments table
^^^^^^^^^^^^^^

This is the table which stores comments and its metadata.
It stores content of comment, posted date, comment which reply to, and so on.

``_key``
  Comment ID

``comment``
  Content of comment

``last_modified``
  Posted date

``replied_to``
  Comment which you reply to someone

``replied_users``
  List of users who you reply to

``hash_tags``
  List of hash tags about comment

``location``
  Posted place (for geolocation)

``posted_by``
  Person who write comment

``favorited_by``
  Indexes for ``favorites`` column in ``Users`` table.
  With this indexes, you can search the person who mark comment as favorite one.

HashTags table
^^^^^^^^^^^^^^

This is the table which stores hash tags for comments.

``_key``
  Hash tag

``hash_index``
  Indexes for ``Comments.hash_tags``.
  With this indexes, you can search list of comments with specified hash tags.

Bigram table
^^^^^^^^^^^^

This is the table which stores indexes for full text search by user information or comments.

``_key``
  Word

``users_index``
  Indexes of user information.
  This column contains indexes of user name (``Users.name``), current location (``Users.location_str``), profile (``Users.description``).

``comment_index``
  Indexes about content of comments (``Comments.comment``).

GeoIndex table
^^^^^^^^^^^^^^

This is the table which stores indexes of location column to search geo location effectively.

``users_location``
  Indexes of location column for Users table

``comments_location``
  Indexes of location column for Comments table

Loading data
------------

Then, load example data.

::

 load --table Users
 [
   {
     "_key": "alice",
     "name": "Alice",
     "follower": ["bob"],
     "favorites": [],
     "location": "152489000x-255829000",
     "location_str": "Boston, Massachusetts",
     "description": "Groonga developer"
   },
   {
     "_key": "bob",
     "name": "Bob",
     "follower": ["alice","charlie"],
     "favorites": ["alice:1","charlie:1"],
     "location": "146249000x-266228000",
     "location_str": "Brooklyn, New York City",
     "description": ""
   },
   {
     "_key": "charlie",
     "name": "Charlie",
     "follower": ["alice","bob"],
     "favorites": ["alice:1","bob:1"],
     "location": "146607190x-267021260",
     "location_str": "Newark, New Jersey",
     "description": "Hmm,Hmm"
   }
 ]

 load --table Comments
 [
   {
     "_key": "alice:1",
     "comment": "I've created micro-blog!",
     "last_modified": "2010/03/17 12:05:00",
     "posted_by": "alice",
   },
   {
     "_key": "bob:1",
     "comment": "First post. test,test...",
     "last_modified": "2010/03/17 12:00:00",
     "posted_by": "bob",
   },
   {
     "_key": "alice:2",
     "comment": "@bob Welcome!!!",
     "last_modified": "2010/03/17 12:05:00",
     "replied_to": "bob:1",
     "replied_users": ["bob"],
     "posted_by": "alice",
   },
   {
     "_key": "bob:2",
     "comment": "@alice Thanks!",
     "last_modified": "2010/03/17 13:00:00",
     "replied_to": "alice:2",
     "replied_users": ["alice"],
     "posted_by": "bob",
   },
   {
     "_key": "bob:3",
     "comment": "I've just used 'Try-Groonga' now! #groonga",
     "last_modified": "2010/03/17 14:00:00",
     "hash_tags": ["groonga"],
     "location": "146566000x-266422000",
     "posted_by": "bob",
   },
   {
     "_key": "bob:4",
     "comment": "I'm come at city of New York for development camp! #groonga #travel",
     "last_modified": "2010/03/17 14:05:00",
     "hash_tags": ["groonga", "travel"],
     "location": "146566000x-266422000",
     "posted_by": "bob",
   },
   {
     "_key": "charlie:1",
     "comment": "@alice @bob I've tried to register!",
     "last_modified": "2010/03/17 15:00:00",
     "replied_users": ["alice", "bob"],
     "location": "146607190x-267021260",
     "posted_by": "charlie",
   }
   {
     "_key": "charlie:2",
     "comment": "I'm at the Museum of Modern Art in NY now!",
     "last_modified": "2010/03/17 15:05:00",
     "location": "146741340x-266319590",
     "posted_by": "charlie",
   }
 ]

``follower`` column and ``favorites`` column in ``Users`` table and ``replied_users`` column in ``Comments`` table are vector column, so specify the value as an array.

``location`` column in ``Users`` table, ``location`` column in ``Comments`` table use GeoPoint type. This type accepts "[latitude]x[longitude]".

``last_modified`` column in ``Comments`` table use Time type.

There are two way to specify the value.
First, specify epoch (seconds since Jan, 1, 1970 AM 00:00:00) directly. In this case, you can specify micro seconds as fractional part.
The value is converted from factional part to the time which is micro seconds based one when data is loaded.
The second, specify the timestamp as string in following format: "(YEAR)/(MONTH)/(DAY) (HOUR):(MINUTE):(SECOND)". In this way, the string is casted to proper micro seconds
when data is loaded.

.. groonga-command
.. table_create --name Users --flags TABLE_HASH_KEY --key_type ShortText
.. table_create --name Comments --flags TABLE_HASH_KEY --key_type ShortText
.. table_create --name HashTags --flags TABLE_HASH_KEY --key_type ShortText
.. table_create --name Bigram --flags TABLE_PAT_KEY --key_type ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. table_create --name GeoIndex --flags TABLE_PAT_KEY --key_type WGS84GeoPoint
.. column_create --table Users --name name --flags COLUMN_SCALAR --type ShortText
.. column_create --table Users --name follower --flags COLUMN_VECTOR --type Users
.. column_create --table Users --name favorites --flags COLUMN_VECTOR --type Comments
.. column_create --table Users --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
.. column_create --table Users --name location_str --flags COLUMN_SCALAR --type ShortText
.. column_create --table Users --name description --flags COLUMN_SCALAR --type ShortText
.. column_create --table Users --name followee --flags COLUMN_INDEX --type Users --source follower
.. column_create --table Comments --name comment --flags COLUMN_SCALAR --type ShortText
.. column_create --table Comments --name last_modified --flags COLUMN_SCALAR --type Time
.. column_create --table Comments --name replied_to --flags COLUMN_SCALAR --type Comments
.. column_create --table Comments --name replied_users --flags COLUMN_VECTOR --type Users
.. column_create --table Comments --name hash_tags --flags COLUMN_VECTOR --type HashTags
.. column_create --table Comments --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
.. column_create --table Comments --name posted_by --flags COLUMN_SCALAR --type Users
.. column_create --table Comments --name favorited_by --flags COLUMN_INDEX --type Users --source favorites
.. column_create --table HashTags --name hash_index --flags COLUMN_INDEX --type Comments --source hash_tags
.. column_create --table Bigram --name users_index --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Users --source name,location_str,description
.. column_create --table Bigram --name comment_index --flags COLUMN_INDEX|WITH_POSITION --type Comments --source comment
.. column_create --table GeoIndex --name users_location --type Users --flags COLUMN_INDEX --source location
.. column_create --table GeoIndex --name comments_location --type Comments --flags COLUMN_INDEX --source location
.. load --table Users
.. [
..   {
..     "_key": "alice",
..     "name": "Alice",
..     "follower": ["bob"],
..     "favorites": [],
..     "location": "152489000x-255829000",
..     "location_str": "Boston, Massachusetts",
..     "description": "Groonga developer"
..   },
..   {
..     "_key": "bob",
..     "name": "Bob",
..     "follower": ["alice","charlie"],
..     "favorites": ["alice:1","charlie:1"],
..     "location": "146249000x-266228000",
..     "location_str": "Brooklyn, New York City",
..     "description": ""
..   },
..   {
..     "_key": "charlie",
..     "name": "Charlie",
..     "follower": ["alice","bob"],
..     "favorites": ["alice:1","bob:1"],
..     "location": "146607190x-267021260",
..     "location_str": "Newark, New Jersey",
..     "description": "Hmm,Hmm"
..   }
.. ]
.. load --table Comments
.. [
..   {
..     "_key": "alice:1",
..     "comment": "I've created micro-blog!",
..     "last_modified": "2010/03/17 12:05:00",
..     "posted_by": "alice",
..   },
..   {
..     "_key": "bob:1",
..     "comment": "First post. test,test...",
..     "last_modified": "2010/03/17 12:00:00",
..     "posted_by": "bob",
..   },
..   {
..     "_key": "alice:2",
..     "comment": "@bob Welcome!!!",
..     "last_modified": "2010/03/17 12:05:00",
..     "replied_to": "bob:1",
..     "replied_users": ["bob"],
..     "posted_by": "alice",
..   },
..   {
..     "_key": "bob:2",
..     "comment": "@alice Thanks!",
..     "last_modified": "2010/03/17 13:00:00",
..     "replied_to": "alice:2",
..     "replied_users": ["alice"],
..     "posted_by": "bob",
..   },
..   {
..     "_key": "bob:3",
..     "comment": "I've just used 'Try-Groonga' now! #groonga",
..     "last_modified": "2010/03/17 14:00:00",
..     "hash_tags": ["groonga"],
..     "location": "146566000x-266422000",
..     "posted_by": "bob",
..   },
..   {
..     "_key": "bob:4",
..     "comment": "I'm come at city of New York for development camp! #groonga #travel",
..     "last_modified": "2010/03/17 14:05:00",
..     "hash_tags": ["groonga", "travel"],
..     "location": "146566000x-266422000",
..     "posted_by": "bob",
..   },
..   {
..     "_key": "charlie:1",
..     "comment": "@alice @bob I've tried to register!",
..     "last_modified": "2010/03/17 15:00:00",
..     "replied_users": ["alice", "bob"],
..     "location": "146607190x-267021260",
..     "posted_by": "charlie",
..   }
..   {
..     "_key": "charlie:2",
..     "comment": "I'm at the Museum of Modern Art in NY now!",
..     "last_modified": "2010/03/17 15:05:00",
..     "location": "146741340x-266319590",
..     "posted_by": "charlie",
..   }
.. ]


Search
------

Let's search micro-blog.

Search users by keyword
^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search micro-blog against multiple column by keyword.
See :doc:`match_columns` to search multiple column at once.

Let's search user from micro-blog's user name, location, description entries.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_keyword.log
.. select --table Users --match_columns name,location_str,description --query "New York" --output_columns _key,name

By using "New York" as searching keyword for user, "Bob" who lives in "New York" is listed in search result.

Search users by geolocation data (GeoPoint)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search users by column data which use type of GeoPoint.
See :doc:`search` about GeoPoint column.

Following example searches users who live in within 20km from specified location.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_user.log
.. select --table Users --filter 'geo_in_circle(location,"146710080x-266315480",20000)' --output_columns _key,name

It shows that "Bob" and "Charlie" lives in within 20 km from station of "Grand Central Terminal".

Search users who follows specific user
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we do reverse resolution of reference relationships which is described at :doc:`index`.

Following examples shows reverse resolution about ``follower`` column of ``Users`` table.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_follower.log
.. select --table Users --query follower:@bob --output_columns _key,name

It shows that "Alice" and "Charlie" follows "Bob".

Search comments by using the value of GeoPoint type
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments which are written within specific location.

Then, we also use drill down which is described at :doc:`drilldown`.
Following example shows how to drill down against search results.
As a result, we get the value of count which is grouped by user, and hash tags respectively.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_drilldown.log
.. select --table Comments --filter 'geo_in_circle(location,"146867000x-266280000",20000)' --output_columns posted_by.name,comment --drilldown hash_tags,posted_by

Above query searches comments which are posted within 20 km from Central Park in city of New York.

As specified range is 20 km, all comments with location are collected.
You know that search results contain 2 #groonga hash tags and one #travel hash tag, and bob and charlie posted 2 comments.

Search comments by keyword
^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments which contains specific keyword.
And more, Let's calculate the value of `_score` which is described at :doc:`search`.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_score.log
.. select --table Comments --query comment:@Now --output_columns comment,_score

By using 'Now' as a keyword, above query returns 2 comments. It also contains count of 'Now' as the value of `_score`.

Search comments by keyword and geolocation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments by specific keyword and geolocation.
By using `--query` and `--filter` option, following query returns records which are matched to both conditions.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_keyword_and_location.log
.. select --table Comments --query comment:@New --filter 'geo_in_circle(location,"146867000x-266280000",20000)' --output_columns posted_by.name,comment --drilldown hash_tags,posted_by

It returns 1 comment which meets both condition.
It also returns result of drilldown. There is 1 comment which is commented by Bob.

Search comments by hash tags
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments which contains specific hash tags.
Let's use reverse resolution of reference relationships.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_hash_tag.log
.. select --table Comments --query hash_tags:@groonga --output_columns posted_by.name,comment --drilldown posted_by

Above query returns 2 comments which contains #groonga hash tag.
It also returns result of drilldown grouped by person who posted it. It shows that there are 2 comments. Bob commented it.

Search comments by user id
^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments which are posted by specific user.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_posted_by.log
.. select --table Comments --query posted_by:bob --output_columns comment --drilldown hash_tags

Above query returns 4 comments which are posted by Bob.
It also returns result of drilldown by hash tags. There are 2 comments which contains #groonga, and 1 comment which contains #travel as hash tag.

Search user's favorite comments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search user's favorite comments.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_favorite.log
.. select --table Users --query _key:bob --output_columns favorites.posted_by,favorites.comment

Above query returns Bob's favorite comments.

Search comments by posted time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section, we search comments by posted time.
See type of `Time` in :doc:`data`.

Let's search comments that posted time are older than specified time.

.. groonga-command
.. include:: ../example/tutorial/micro_blog_last_modified.log
.. select Comments --filter 'last_modified<=1268802000' --output_columns posted_by.name,comment,last_modified --drilldown hash_tags,posted_by

Above query returns 5 comments which are older than 2010/03/17 14:00:00.
It also returns result of drilldown by posted person. There are 2 comments by Alice, 3 comments by Bob.

.. TODO: 以下の機能はgroonga本体での支援が必要。
.. タイムライン表示: あるユーザがfollowしているユーザの発言を、時系列順の逆順で並べて10件のみ表示。
.. 複数の条件での検索: favoriteの数がn以上の発言で全文検索、さらにユーザでドリルダウン
