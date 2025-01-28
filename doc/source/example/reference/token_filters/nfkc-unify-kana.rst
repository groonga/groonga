Execution example::

  tokenize TokenDelimit "あイｳｪおヽヾ" --token_filters 'TokenFilterNFKC("unify_kana", true)'
  # [
  #   [
  #     0,
  #     1337566253.89858,
  #     0.000355720520019531
  #   ],
  #   [
  #     {
  #       "value": "あいうぇおゝゞ",
  #       "position": 0,
  #       "force_prefix": false,
  #       "force_prefix_search": false
  #     }
  #   ]
  # ]
