Execution example:

```shell
table_create Data TABLE_NO_KEY
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Data value COLUMN_SCALAR JSON
# [[0,1337566253.89858,0.000355720520019531],true]
table_create Numbers TABLE_PAT_KEY Int32 \
  --extractors 'ExtractorJSON("path", "$.value[*][*]")'
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Numbers data_value COLUMN_INDEX Data value
# [[0,1337566253.89858,0.000355720520019531],true]
load --table Data
[
{"value": "{\"value\": [[1, 10], [100]]}"},
{"value": "{\"value\": [[2], [20, 200]]}"},
{"value": "{\"value\": [[-1, -10], [-100]]}"}
]
# [[0,1337566253.89858,0.000355720520019531],3]
select Data --filter 'between(Numbers.data_value, 10, 20)'
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
