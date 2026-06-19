Execution example:

```shell
select Memos \
  --filter 'language_model_knn(text, "male child", { "k" : -1 })' \
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
#         11
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
#         "This is a pepper."
#       ],
#       [
#         "This is a cucumber."
#       ],
#       [
#         "This is a carrot."
#       ],
#       [
#         "This is a potato."
#       ],
#       [
#         "This is an apple."
#       ],
#       [
#         "This is a banana."
#       ],
#       [
#         "This is a tomato."
#       ],
#       [
#         "This is an orange."
#       ],
#       [
#         "This is an onion."
#       ]
#     ]
#   ]
# ]
```
