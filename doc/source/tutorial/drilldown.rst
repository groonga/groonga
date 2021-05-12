.. -*- rst -*-
.. Groonga Project

.. groonga-include : search.rst

.. groonga-command
.. database: tutorial

Drilldown
=========

You learned how to search and sort searched results in the previous sections.
Now that you can search as you likes, but how do you summarize the number of records which has specific value in the column?

As you know, there is a naive solution to execute query by every the value of column, then you can get the number of records as a result. It is a simple way, but it is not reasonable to many records.

If you are familiar with SQL, you will doubt with "Is there a similar SQL functionality to ``GROUP BY`` in Groonga?".

Of course, Groonga provides such a functionality. It's called as ``drilldown``.

``drilldown`` enables you to get the number of records which belongs to specific the value of column at once.

To illustrate this feature, imagine the case that classification by domain and grouping by country that domain belongs to.

Here is the concrete examples how to use this feature.

In this example, we add two columns to ``Site`` table. ``domain`` column is used for TLD (top level domain). ``country`` column is used for country name. The type of these columns are ``SiteDomain`` table which uses domain name as a primary key and ``SiteCountry`` table which uses country name as a primary key.

.. groonga-command
.. include:: ../example/tutorial/drilldown-1.log
.. table_create --name SiteDomain --flags TABLE_HASH_KEY --key_type ShortText
.. table_create --name SiteCountry --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Site --name domain --flags COLUMN_SCALAR --type SiteDomain
.. column_create --table Site --name country --flags COLUMN_SCALAR --type SiteCountry
.. load --table Site
.. [
.. {"_key":"http://example.org/","domain":".org","country":"japan"},
.. {"_key":"http://example.net/","domain":".net","country":"brazil"},
.. {"_key":"http://example.com/","domain":".com","country":"japan"},
.. {"_key":"http://example.net/afr","domain":".net","country":"usa"},
.. {"_key":"http://example.org/aba","domain":".org","country":"korea"},
.. {"_key":"http://example.com/rab","domain":".com","country":"china"},
.. {"_key":"http://example.net/atv","domain":".net","country":"china"},
.. {"_key":"http://example.org/gat","domain":".org","country":"usa"},
.. {"_key":"http://example.com/vdw","domain":".com","country":"japan"}
.. ]

Here is a example of drilldown with ``domain`` column. Three kind of values are used in ``domain`` column - ".org", ".net" and ".com".

.. groonga-command
.. include:: ../example/tutorial/drilldown-domain.log
.. select --table Site --limit 0 --drilldown domain


Here is a summary of above query.

.. list-table:: Drilldown by ``domain`` column
   :header-rows: 1

   * - Group by
     - The number of group records
     - Group records means following records
   * - .org
     - 3
     - * http://example.org/
       * http://example.org/aba
       * http://example.org/gat
   * - .net
     - 3
     - * http://example.net/
       * http://example.net/afr
       * http://example.net/atv
   * - .com
     - 3
     - * http://example.com/
       * http://example.com/rab
       * http://example.com/vdw


The value of drilldown are returned as the value of ``_nsubrecs`` column. In this case, ``Site`` table is grouped by ".org", ".net", ".com" domain. ``_nsubrecs`` shows that each three domain has three records.

If you execute drildown to the column which has table as a type, you can get the value of column which is stored in referenced table.
``_nsubrecs`` pseudo column is added to the table which is used for drilldown. this pseudo column stores the number of records which is grouped by.

Then, investigate referenced table in detail. As ``Site`` table use ``SiteDomain`` table as column type of ``domain``, you can use ``--drilldown_output_columns`` to know detail of referenced column.

.. groonga-command
.. include:: ../example/tutorial/drilldown-output-columns.log
.. select --table Site --limit 0 --drilldown domain --drilldown_output_columns _id,_key,_nsubrecs

Now, you can see detail of each grouped domain, drilldown by ``country`` column which has ".org" as column value.

.. groonga-command
.. include:: ../example/tutorial/drilldown-country.log
.. select --table Site --limit 0 --filter "domain._id == 1" --drilldown country

Drilldown with multiple column
------------------------------

Drilldown feature supports multiple column. Use comma separated multiple column names as ``drildown`` parameter.
You can get the each result of drilldown at once.

.. groonga-command
.. include:: ../example/tutorial/drilldown-multiple-column.log
.. select --table Site --limit 0 --drilldown domain,country


Sorting drildown results
------------------------

Use ``--drilldown_sort_keys`` if you want to sort the result of drilldown. For example, specify ``_nsubrecs`` as ascending order.

.. groonga-command
.. include:: ../example/tutorial/drilldown-sortby.log
.. select --table Site --limit 0 --drilldown country --drilldown_sort_keys _nsubrecs


limits drildown results
------------------------

The number of drilldown results is limited to 10 as a default. Use ``drilldown_limit`` and ``drilldown_offset`` parameter to customize orilldown results.

.. groonga-command
.. include:: ../example/tutorial/drilldown-limit.log
.. select --table Site --limit 0 --drilldown country --drilldown_sort_keys _nsubrecs --drilldown_limit 2 --drilldown_offset 2

Note that drilldown to the column which stores string is slower than the columns which stores the other types.
If you drilldown to string type of column, create the table that type of primary key is string, then create the column which refers that table.
