Execution example:

```shell
select \
  --table Memos \
  --columns[similarity].stage filtered \
  --columns[similarity].flags COLUMN_SCALAR \
  --columns[similarity].types Float32 \
  --columns[similarity].value 'distance_inner_product(content_embedding, language_model_vectorize("mistral-7b-v0.1.Q4_K_M", "high performance FTS"))' \
  --output_columns content,similarity \
  --sort_keys -similarity
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
#         ],
#         [
#           "similarity",
#           "Text"
#         ]
#       ],
#       [
#         "Groonga is fast and embeddable full text search engine.",
#         "0.6581704020500183"
#       ],
#       [
#         "PGroonga is a PostgreSQL extension that uses Groonga.",
#         "0.6540993452072144"
#       ],
#       [
#         "PostgreSQL is a RDBMS.",
#         "0.6449499130249023"
#       ]
#     ]
#   ]
# ]
```
