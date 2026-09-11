Execution example:

```shell
select Memos \
  --output_columns '_key, string_truncate(_key, 100)'
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   [
#     [
#       [
#         1
#       ],
#       [
#         [
#           "_key",
#           "ShortText"
#         ],
#         [
#           "string_truncate",
#           null
#         ]
#       ],
#       [
#         "Groonga is a full text search engine",
#         "Groonga is a full text search engine"
#       ]
#     ]
#   ]
# ]
```
