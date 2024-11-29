# Patricia trie defragmentation: Test cases

Common: Defragmentation should reduce `total_size`.

## Reduced of `total_size`

A simple defragmentation test to check if the size has been reduced.

* test/command/suite/defrag/pat/multiple_segments/remainder_in_segment.test
  * Test with big data using multiple segments
  * Test using the not full area per segment
* test/command/suite/defrag/pat/multiple_segments/use_full_segment.test
  * Test with big data using multiple segments
  * Test using the full area per segment

## Zero data

* test/command/suite/defrag/pat/empty.test
  * If there are zero data, `defrag` should exit normally

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

## `_id` reused

Defragmentation should succeed if the key buffer is no longer in `_id` order because `_id` is reused.

* test/command/suite/defrag/pat/reuse_id.test
