Execution example:

```shell
select Memos \
  --output_columns 'string_truncate("Groonga is a fast fulltext search engine", 15)'
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
#           "string_truncate",
#           null
#         ]
#       ],
#       [
#         "Groonga is a..."
#       ]
#     ]
#   ]
# ]
```
