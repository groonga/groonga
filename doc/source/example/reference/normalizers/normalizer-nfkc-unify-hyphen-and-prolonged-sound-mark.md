Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_hyphen_and_prolonged_sound_mark", true)' \
  "-˗֊‐‑‒–⁃⁻₋− ﹣－ ー—―─━ｰ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "----------- -- ------",
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
#       "others",
#       "symbol",
#       "symbol",
#       "others",
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
