Execution example::

  normalize   'NormalizerNFKC("unify_katakana_zu_small_sounds", true)'   "ズァズィズェズォ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "ザジゼゾ",
  #     "types": [
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
