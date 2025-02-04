Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_katakana_v_sounds", true)' \
  "ヴァヴィヴヴェヴォヴ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "バビブベボブ",
#     "types": [
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
