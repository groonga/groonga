.. -*- rst -*-

.. groonga-command
.. database: tutorial-patricia-trie

Prefix search with patricia trie
================================

Groonga supports to create a table with patricia trie option.
By specifying it, You can do prefix search.

And more, you can do suffix search against primary key by specifying additional option.

Prefix search by primary key
----------------------------

table_create command which uses TABLE_PAT_KEY for flags option supports prefix search by primary key.

.. groonga-command
.. include:: ../example/tutorial/patricia_trie_prefix_search.log
..
.. table_create --name PatPrefix --flags TABLE_PAT_KEY --key_type ShortText
.. load --table PatPrefix
.. [
.. {"_key":"James"}
.. {"_key":"Jason"}
.. {"_key":"Jennifer"},
.. {"_key":"Jeff"},
.. {"_key":"John"},
.. {"_key":"Joseph"},
.. ]
.. select --table PatPrefix --query _key:^Je

Suffix search by primary key
----------------------------

table_create command which uses TABLE_PAT_KEY and KEY_WITH_SIS for flags option supports prefix search and suffix search by primary key.

If you set KEY_WITH_SIS flag, suffix search records also are added when you add the data. So if you search simply, the automatically added records are hit in addition to the original records. In order to search only the original records, you need a plan.

For example, in order to make this distinction between the original records and automatically added records, add the original column indicating that it is the original record, and add original column is ``true`` to the search condition. For attention, use ``--filter`` option because ``--query`` option is not specify ``Bool`` type value intuitively.

.. groonga-command
.. include:: ../example/tutorial/patricia_trie-2.log
..
.. table_create --name PatSuffix --flags TABLE_PAT_KEY|KEY_WITH_SIS --key_type ShortText
.. column_create --table PatSuffix --name original --type Bool
.. load --table PatSuffix
.. [
.. {"_key":"ひろゆき","original":true},
.. {"_key":"まろゆき","original":true},
.. {"_key":"ひろあき","original":true},
.. {"_key":"ゆきひろ","original":true}
.. ]
.. select --table PatSuffix --query _key:$ゆき
.. select --table PatSuffix --filter '_key @$ "ゆき" && original == true'
