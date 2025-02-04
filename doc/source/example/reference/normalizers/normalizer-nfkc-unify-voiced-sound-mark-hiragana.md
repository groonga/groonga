Execution example:

```shell
normalize \
  'NormalizerNFKC("unify_kana_voiced_sound_mark", true)' \
  "かがきぎくぐけげこごさざしじすずせぜそぞただちぢつづてでとどはばぱひびぴふぶぷへべぺほぼぽ" \
  WITH_TYPES
# [
#   [
#     0,
#     1337566253.89858,
#     0.000355720520019531
#   ],
#   {
#     "normalized": "かかききくくけけここささししすすせせそそたたちちつつててととはははひひひふふふへへへほほほ",
#     "types": [
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "hiragana",
#       "null"
#     ],
#     "checks": []
#   }
# ]
```
