Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_to_katakana", true)' \
  "ゔぁゔぃゔゔぇゔぉ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "ヴァヴィヴヴェヴォ",
#     "types": [
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
