Execution example::

  normalize   'NormalizerNFKC("unify_katakana_bu_sound", true)'   "ヴァヴィヴヴェヴォヴ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "ブブブブブブ",
  #     "types": [
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
