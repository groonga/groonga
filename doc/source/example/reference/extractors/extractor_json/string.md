Execution example:

```shell
extract \
  --extractors 'ExtractorJSON("path", "$.tags[*]")' \
  --value '{"tags": ["groonga", "search", "engine"], "title": "ignored"}'
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "extracted": [
#       "groonga",
#       "search",
#       "engine"
#     ]
#   }
# ]
```
