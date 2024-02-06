window.BENCHMARK_DATA = {
  "lastUpdate": 1707194613174,
  "repoUrl": "https://github.com/groonga/groonga",
  "entries": {
    "Benchmark": [
      {
        "commit": {
          "author": {
            "email": "kou@clear-code.com",
            "name": "Sutou Kouhei",
            "username": "kou"
          },
          "committer": {
            "email": "kou@clear-code.com",
            "name": "Sutou Kouhei",
            "username": "kou"
          },
          "distinct": true,
          "id": "40177b47121d6fa276f4fad7142fc68e7989a50c",
          "message": "ci benchmark: remove a needless option",
          "timestamp": "2024-02-05T18:17:58+09:00",
          "tree_id": "7eba45cecf008062f2e21fcc07078242e650d861",
          "url": "https://github.com/groonga/groonga/commit/40177b47121d6fa276f4fad7142fc68e7989a50c"
        },
        "date": 1707125047570,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/short_text",
            "value": 0.02649119700004121,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0014410000000001366 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "kou@clear-code.com",
            "name": "Sutou Kouhei",
            "username": "kou"
          },
          "committer": {
            "email": "kou@clear-code.com",
            "name": "Sutou Kouhei",
            "username": "kou"
          },
          "distinct": true,
          "id": "7badc9df37f11cde4d10e0a115b41e85df4c8b3e",
          "message": "benchmark: add multiple columns case",
          "timestamp": "2024-02-05T18:36:32+09:00",
          "tree_id": "a432bcba536ec7aa72bc3d6464602010f3d9c61f",
          "url": "https://github.com/groonga/groonga/commit/7badc9df37f11cde4d10e0a115b41e85df4c8b3e"
        },
        "date": 1707132685278,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.03835392100000945,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002371000000000123 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.026424021000025277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001549999999999968 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "abe@clear-code.com",
            "name": "Abe Tomoaki",
            "username": "abetomo"
          },
          "committer": {
            "email": "abe@clear-code.com",
            "name": "Abe Tomoaki",
            "username": "abetomo"
          },
          "distinct": true,
          "id": "03286ef39c2af3312cc984fd1f937a25f3468fa9",
          "message": "Use bool instead of grn_bool in store.{c,h}\n\nhttps://github.com/groonga/groonga/issues/1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-06T08:33:19+09:00",
          "tree_id": "01249d3cfc9d521ff9f094c82b2b818ebbcfdac3",
          "url": "https://github.com/groonga/groonga/commit/03286ef39c2af3312cc984fd1f937a25f3468fa9"
        },
        "date": 1707178570830,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.035824695000030715,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018400000000000083 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.026267163999989407,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0014630000000002141 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "abe@clear-code.com",
            "name": "Abe Tomoaki",
            "username": "abetomo"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f851bb12b33c576b9de999e4c35cb80d165ea9a3",
          "message": "Remove `init_default_hostname` and always use `localhost` as the default hostname (#1694)\n\nGitHub: fix GH-1644\r\n\r\n`init_default_hostname()` is slow on Mac.\r\nRemove `init_default_hostname()` and always use `localhost` as the\r\ndefault hostname.\r\n\r\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-06T13:38:57+09:00",
          "tree_id": "ab6b54878ff5078c23f840d493d77ef774c35ae1",
          "url": "https://github.com/groonga/groonga/commit/f851bb12b33c576b9de999e4c35cb80d165ea9a3"
        },
        "date": 1707194611542,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.03677418099999841,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002019000000000215 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.026499073999957545,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017179999999998863 s\nthreads: undefined"
          }
        ]
      }
    ]
  }
}