Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_middle_dot", true)' \
  "·ᐧ•∙⋅⸱・･" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "········",
#     "types": [
#       "symbol",
#       "symbol",
#       "symbol",
#       "symbol",
#       "symbol",
#       "symbol",
#       "symbol",
#       "symbol",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
