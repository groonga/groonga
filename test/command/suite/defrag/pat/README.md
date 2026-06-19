# Patricia trie defragmentation: Test cases

## Common checkpoints

* Defragmentation should reduce `total_size`
* Test with data that includes the keys immediate=true and immediate=false
  * immediate=true
    * Key length is less than or equal to 4 bytes
  * immediate=false
    * Key length is greater than or equal to 5 bytes

## Reduced of `total_size`

A simple defragmentation test to check if the size has been reduced.

* test/command/suite/defrag/pat/multiple_segments/remainder_in_segment.test
  * Test with big data using multiple segments
  * Test using the not full area per segment
* test/command/suite/defrag/pat/multiple_segments/use_full_segment.test
  * Test with big data using multiple segments
  * Test using the full area per segment

## Zero data

If there are zero data, `defrag` should exit normally.

* test/command/suite/defrag/pat/empty/after_creation.test
  * Defragmentation on tables with no registration
* test/command/suite/defrag/pat/empty/delete_all.test
  * Defragmentation of a table with zero data after registering and deleting all data

## Key buffer overlaps in defragmentation

* test/command/suite/defrag/pat/area_overlap.test
  * When defragmenting, it overlaps itself with the area where it was originally located

Case where "James" is deleted from the "before:" state and the "Jason-TEST" area is overlapped and defragmented.

```
before: |James|Jason-TEST|..|
after : |Jason-TEST|..|
```

## Load after `defrag`

Test if data is successfully added after defragmentation.

* test/command/suite/defrag/pat/after_load.test
  * Test reduction with less data
* test/command/suite/defrag/pat/multiple_segments/after_load.test
  * Test with big data using multiple segments

## Defragment a few times

Defragmentation should succeed no matter how many times it is run.

* test/command/suite/defrag/pat/repeat_load_defrag_delete.test

## `_id` reused

Defragmentation should succeed even if the `_id` is reused.

* test/command/suite/defrag/pat/reuse_id.test

FYI:
Reuse is a form of reusing a node that has already been deleted.
`_id` and key storage for that node is reuse as it is, so the key storage is in `_id` order.

## No fragmentation

Defragmentation on a table with no fragmentation finishes successfully.

* test/command/suite/defrag/pat/no_fragmentation.test

## The garbage is cleared after defragmentation

To avoid the incorrect reuse of the node, the garbage is cleared during defragmentation.
Test whether the garbage was successfully cleared after defragmentation
and whether it can be reused successfully after defragmentation.

### With node reuse before defragmentation

All records in the target table are deleted and empty.

* test/command/suite/defrag/pat/clear_garbage/reuse_before_defrag/emtpy/not_reuse_after_defrag.test
  * After defragmentation, there is no garbage and no reuse
* test/command/suite/defrag/pat/clear_garbage/reuse_before_defrag/emtpy/reuse_after_defrag.test
  * Even after defragmentation, deleting more than 256 nodes occurs reuse

A few records remain.

* test/command/suite/defrag/pat/clear_garbage/reuse_before_defrag/not_reuse_after_defrag.test
  * After defragmentation, there is no garbage and no reuse
* test/command/suite/defrag/pat/clear_garbage/reuse_before_defrag/reuse_after_defrag.test
  * Even after defragmentation, deleting more than 256 nodes occurs reuse

### Without node reuse before defragmentation

All records in the target table are deleted and empty.

* test/command/suite/defrag/pat/clear_garbage/no_reuse_before_defrag/emtpy/not_reuse_after_defrag.test
  * After defragmentation, there is no garbage and no reuse
* test/command/suite/defrag/pat/clear_garbage/no_reuse_before_defrag/emtpy/reuse_after_defrag.test
  * Even after defragmentation, deleting more than 256 nodes occurs reuse

A few records remain.

* test/command/suite/defrag/pat/clear_garbage/no_reuse_before_defrag/not_reuse_after_defrag.test
  * After defragmentation, there is no garbage and no reuse
* test/command/suite/defrag/pat/clear_garbage/no_reuse_before_defrag/reuse_after_defrag.test
  * Even after defragmentation, deleting more than 256 nodes occurs reuse

## Overwrite the already created ID

If the max value of the created `_id` has already been deleted at the time of defragmentation, `_id` can be reused.

Example:

`_id=1~10` have been created.

* If `_id=1,3` have already been deleted
  * `_id=1,3` cannot be reused
  * Do not overwrite the already created record ID
* If `_id=10` have already been deleted
  * `_id=10` can be reused
  * Overwrite the already created ID with 9
    *  Before the overwrite, it was 10
* If `_id=9,10` have already been deleted
  * `_id=9,10` can be reused
  * Overwrite the already created ID with 8
    *  Before the overwrite, it was 10

Tests:

* test/command/suite/defrag/pat/overwrite_curr_rec/empty/after_creation.test
  * Defragmentation on tables with no registration
  * Overwrite the already created ID with 0
* test/command/suite/defrag/pat/overwrite_curr_rec/empty/delete_all.test
  * Defragmentation of a table with zero data after registering and deleting all data
  * Overwrite the already created ID with 0
* test/command/suite/defrag/pat/overwrite_curr_rec/max_id_active/immediate.test
  * The max `_id` is active and will not be overwritten
  * The max `_id` is immediate=true
* test/command/suite/defrag/pat/overwrite_curr_rec/max_id_active/not_immediate.test
  * The max `_id` is active and will not be overwritten
  * The max `_id` is immediate=false
* test/command/suite/defrag/pat/overwrite_curr_rec/max_id_deleted/immediate.test
  * The max `_id` has already deleted
  * The max `_id` is immediate=true
  * Overwrite the already created ID with the max active `_id`
* test/command/suite/defrag/pat/overwrite_curr_rec/max_id_deleted/not_immediate.test
  * The max `_id` has already deleted
  * The max `_id` is immediate=false
  * Overwrite the already created ID with the max active `_id`
