Execution example::

  normalize   'NormalizerNFKC("unify_katakana_di_sound", true)'   "ヂ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "ジ",
  #     "types": [
  #       "katakana",
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
