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

## No fragmentation

Defragmentation on a table with no fragmentation finishes successfully.

* test/command/suite/defrag/pat/no_fragmentation.test
