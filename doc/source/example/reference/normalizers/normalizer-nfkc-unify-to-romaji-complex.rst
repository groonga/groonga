Execution example::

  tokenize 'TokenMecab("target_class", "-名詞/非自立", "target_class", "-名詞/接尾/人名", "target_class", "名詞", "include_reading", true)' '彼の名前は山田さんのはずです。'
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   [
  #     {
  #       "value": "彼",
  #       "position": 0,
  #       "force_prefix": false,
  #       "force_prefix_search": false,
  #       "metadata": {
  #         "reading": "カレ"
  #       }
  #     },
  #     {
  #       "value": "名前",
  #       "position": 1,
  #       "force_prefix": false,
  #       "force_prefix_search": false,
  #       "metadata": {
  #         "reading": "ナマエ"
  #       }
  #     },
  #     {
  #       "value": "山田",
  #       "position": 2,
  #       "force_prefix": false,
  #       "force_prefix_search": false,
  #       "metadata": {
  #         "reading": "ヤマダ"
  #       }
  #     }
  #   ]
  # ]
  normalize   'NormalizerNFKC("unify_to_romaji", true)'   "カレ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "kare",
  #     "types": [
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
  normalize   'NormalizerNFKC("unify_to_romaji", true)'   "ナマエ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "namae",
  #     "types": [
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
  normalize   'NormalizerNFKC("unify_to_romaji", true)'   "ヤマダ"   WITH_TYPES
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   {
  #     "normalized": "yamada",
  #     "types": [
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "alpha",
  #       "null"
  #     ],
  #     "checks": [
  # 
  #     ]
  #   }
  # ]
