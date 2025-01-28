Execution example::

  normalize   'NormalizerNFKC("unify_kana", true)'   "あイｳｪおヽヾ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "あいうぇおゝゞ",
  #     "types": [
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
