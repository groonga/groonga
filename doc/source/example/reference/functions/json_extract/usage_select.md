Execution example:

```shell
select Data --filter 'between(json_extract(value, "$.value[*][*]"), 10, 20)'
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   [
#     [
#       [
#         2
#       ],
#       [
#         [
#           "_id",
#           "UInt32"
#         ],
#         [
#           "value",
#           "JSON"
#         ]
#       ],
#       [
#         1,
#         {
#           "value": [
#             [
#               1,
#               10
#             ],
#             [
#               100
#             ]
#           ]
#         }
#       ],
#       [
#         2,
#         {
#           "value": [
#             [
#               2
#             ],
#             [
#               20,
#               200
#             ]
#           ]
#         }
#       ]
#     ]
#   ]
# ]
```
