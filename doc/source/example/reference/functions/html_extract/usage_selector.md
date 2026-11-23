Execution example:

```shell
select Pages --output_columns 'html_extract(html, {"selector": "article"})'
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
#           "html_extract",
#           null
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
