Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_iteration_mark", true)' \
  "時々" \
  WITH_TYPES
# [
#   [
#     0,
#     1758882899.892991,
#     0.0004293918609619141
#   ],
#   {
#     "normalized": "時時",
#     "types": [
#       "kanji",
#       "kanji",
#       "null"
#     ],
#     "checks": [
#     ]
#   }
# ]
```
