Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_katakana_wo_sound", true)' \
  "ヲ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "オ",
#     "types": [
#       "katakana",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
