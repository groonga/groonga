Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_to_romaji", true)' \
  "アァイィウゥエェオォ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "axaixiuxuexeoxo",
#     "types": [
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
