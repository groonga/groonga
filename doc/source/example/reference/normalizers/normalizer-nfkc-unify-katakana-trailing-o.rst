Execution example::

  normalize   'NormalizerNFKC("unify_katakana_trailing_o", true)'   "オオコオソオトオノオ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "オウコウソウトウノウ",
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
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
