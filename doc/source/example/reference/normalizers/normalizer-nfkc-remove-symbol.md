Execution example:

```shell
normalize \
  'NormalizerNFKC("remove_symbol", true)' \
  "#This & is %% a pen." \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "this  is  a pen",
#     "types": [
#       "alpha",
#       "alpha",
#       "alpha",
#       "alpha",
#       "others",
#       "others",
#       "alpha",
#       "alpha",
#       "others",
#       "others",
#       "alpha",
#       "others",
#       "alpha",
#       "alpha",
#       "alpha",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
