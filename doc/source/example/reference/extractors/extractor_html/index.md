Execution example:

```shell
table_create Contents TABLE_NO_KEY
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Contents html COLUMN_SCALAR Text
# [[0,1337566253.89858,0.000355720520019531],true]
table_create Terms TABLE_PAT_KEY ShortText \
  --default_tokenizer TokenBigram \
  --normalizers NormalizerNFKC \
  --extractors ExtractorHTML
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Terms contents_html COLUMN_INDEX|WITH_POSITION Contents html
# [[0,1337566253.89858,0.000355720520019531],true]
load --table Contents
[
{"html": "<p>Groonga is a <b>fast</b> full text search engine.</p>"}
]
# [[0,1337566253.89858,0.000355720520019531],1]
select Contents \
  --match_columns html \
  --query "fast" \
  --output_columns html
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
#           "html",
#           "Text"
#         ]
#       ],
#       [
#         "<p>Groonga is a <b>fast</b> full text search engine.</p>"
#       ]
#     ]
#   ]
# ]
select Contents \
  --match_columns html \
  --query "<b>" \
  --output_columns html
# [[0,1337566253.89858,0.000355720520019531],[[[0],[["html","Text"]]]]]
```
