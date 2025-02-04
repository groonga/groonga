Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_prolonged_sound_mark", true)' \
  "ー—―─━ｰ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "ーーーーーー",
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
