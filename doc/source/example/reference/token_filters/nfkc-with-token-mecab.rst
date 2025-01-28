Execution example::

  tokenize   'TokenMecab("use_reading", true)'   "私は林檎を食べます。"   --token_filters 'TokenFilterNFKC("unify_kana", true)'
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   [
  #     {
  #       "value": "わたし",
  #       "position": 0,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "は",
  #       "position": 1,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "りんご",
  #       "position": 2,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "を",
  #       "position": 3,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "たべ",
  #       "position": 4,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "ます",
  #       "position": 5,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     },
  #     {
  #       "value": "。",
  #       "position": 6,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     }
  #   ]
  # ]
