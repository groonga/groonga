Execution example:

```shell
column_create Pages content COLUMN_SCALAR Text html \
  --generator 'html_extract(html, {"selector": "article"})'
# [[0,1337566253.89858,0.000355720520019531],true]
select Pages --output_columns content
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
#           "content",
#           "Text"
#         ]
#       ],
#       [
#         ""
#       ],
#       [
#         "Hello"
#       ]
#     ]
#   ]
# ]
```
