Execution example::

  normalize   'NormalizerNFKC("unify_kana_case", true)'   "ぁあぃいぅうぇえぉおゃやゅゆょよゎわゕかゖけ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "ああいいううええおおややゆゆよよわわかかけけ",
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
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
