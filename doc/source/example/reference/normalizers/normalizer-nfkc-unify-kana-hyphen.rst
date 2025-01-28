Execution example::

  normalize   'NormalizerNFKC("unify_kana_hyphen", true)'   "カ-キ-ク-ケ-コ-"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "カアキイクウケエコオ",
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
