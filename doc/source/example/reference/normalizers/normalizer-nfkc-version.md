Execution example:

```shell
normalize 'NormalizerNFKC("version", "16.0.0")' "©" WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "©",
#     "types": [
#       "emoji",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
