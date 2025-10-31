Execution example:

```shell
select Memos \
  --filter 'language_model_knn(content, "male child")' \
  --output_columns content
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   [
#     [
#       [
#         3
#       ],
#       [
#         [
#           "content",
#           "ShortText"
#         ]
#       ],
#       [
#         "I am a boy."
#       ],
#       [
#         "This is an apple."
#       ],
#       [
#         "Groonga is a full text search engine."
#       ]
#     ]
#   ]
# ]
```
