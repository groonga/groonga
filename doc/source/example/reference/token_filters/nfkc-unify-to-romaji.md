Execution example:

```shell
tokenize \
  TokenDelimit \
  "アァイィウゥエェオォ" \
  --token_filters  'TokenFilterNFKC("unify_to_romaji", true)'
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   [
#     {
#       "value": "axaixiuxuexeoxo",
#       "position": 0,
#       "force_prefix": false,
#       "force_prefix_search": false
#     }
#   ]
# ]
```
