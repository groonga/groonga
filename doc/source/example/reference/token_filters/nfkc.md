Execution example:

```shell
tokenize TokenDelimit "©" --token_filters TokenFilterNFKC
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   [
#     {
#       "value": "©",
#       "position": 0,
#       "force_prefix": false,
#       "force_prefix_search": false
#     }
#   ]
# ]
```
