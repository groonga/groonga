Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_hyphen", true)' \
  "-˗֊‐‑‒–⁃⁻₋−" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "-----------",
#     "types": [
#       "symbol",
#       "symbol",
#       "symbol",
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
