Execution example::

  normalize   'NormalizerNFKC("unify_katakana_du_sound", true)'   "ヅ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "ズ",
  #     "types": [
  #       "katakana",
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
