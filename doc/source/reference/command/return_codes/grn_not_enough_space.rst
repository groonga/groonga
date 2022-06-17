.. -*- rst -*-

``-13: GRN_NOT_ENOUGH_SPACE``
=============================

Major cause
-----------

1. If the total key size is over the limit of a hash table or a patricia trie table.

   please see :doc:`/limitations` for the detail of these limitations.

2. If the space to store the index is not enough.

Major action on this error
--------------------------

1. If the total key size is over the limit of a hash table or a patricia trie table.

   We reduce the number of records of the target table by such as a split table.

2. If the space to store the index is not enough.

   We reduce the number of records that set the target index by such as a split table.
