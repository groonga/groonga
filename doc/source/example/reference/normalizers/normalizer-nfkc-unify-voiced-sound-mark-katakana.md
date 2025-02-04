Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_kana_voiced_sound_mark", true)' \
  "カガキギクグケゲコゴサザシジスズセゼソゾタダチヂツヅテデトドハバパヒビピフブプヘベペホボポ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "カカキキククケケココササシシススセセソソタタチチツツテテトトハハハヒヒヒフフフヘヘヘホホホ",
#     "types": [
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "katakana",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
