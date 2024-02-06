window.BENCHMARK_DATA = {
  "lastUpdate": 1707262673699,
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
          "id": "8fbc284aeb2282073a11d172a482bd5e9a5a02dc",
          "message": "command: make \"n_workers\" as a common parameter\n\nIt's only used by select for now.\n\nGRN_SELECT_N_WORKERS_DEFAULT is still usable for backward\ncompatibility but it's deprecated. Use GRN_N_WORKERS_DEFAULT instead.",
          "timestamp": "2024-02-06T14:38:50+09:00",
          "tree_id": "edad0f7a5281ac2e45254e4dbd286781fb0487e9",
          "url": "https://github.com/groonga/groonga/commit/8fbc284aeb2282073a11d172a482bd5e9a5a02dc"
        },
        "date": 1707198259778,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.03688777000002119,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016819999999999335 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.028147521999983383,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021219999999997075 s\nthreads: undefined"
          },
          {
            "name": "select/olap/multiple",
            "value": 0.018036934000008387,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005509999999997461 s\nthreads: undefined"
          },
          {
            "name": "select/olap/n_workers/multiple",
            "value": 0.026510713999982727,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0007190000000003305 s\nthreads: undefined"
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
          "id": "05a348984d16fc6a2fd148c90b80fe947afd8d1c",
          "message": "Fix style",
          "timestamp": "2024-02-06T14:42:41+09:00",
          "tree_id": "e6feff1784ee73ac91e421daacc9cd087aaf45f5",
          "url": "https://github.com/groonga/groonga/commit/05a348984d16fc6a2fd148c90b80fe947afd8d1c"
        },
        "date": 1707198472157,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.03902332899997418,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0023649999999998395 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.02650554999996757,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0015210000000002166 s\nthreads: undefined"
          },
          {
            "name": "select/olap/multiple",
            "value": 0.018209037999952216,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005260000000006926 s\nthreads: undefined"
          },
          {
            "name": "select/olap/n_workers/multiple",
            "value": 0.014382067999974879,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005809999999996374 s\nthreads: undefined"
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
          "id": "ed84655236af474a1eee48800698dd51bec6fa61",
          "message": "TaskExecutor: use ID directly",
          "timestamp": "2024-02-06T21:39:57+09:00",
          "tree_id": "bddc5ba6f195bb1f8bddf38fc1f90989d4012c35",
          "url": "https://github.com/groonga/groonga/commit/ed84655236af474a1eee48800698dd51bec6fa61"
        },
        "date": 1707223634679,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "load/data/multiple",
            "value": 0.036709383000015805,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0024490000000001177 s\nthreads: undefined"
          },
          {
            "name": "load/data/short_text",
            "value": 0.02570906300002207,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0014210000000001166 s\nthreads: undefined"
          },
          {
            "name": "select/olap/multiple",
            "value": 0.01717463099998895,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0006029999999999092 s\nthreads: undefined"
          },
          {
            "name": "select/olap/n_workers/multiple",
            "value": 0.016579061000015827,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004940000000000777 s\nthreads: undefined"
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
          "id": "0fc3552069ef94d1eaf1f19fdef6917d67155dd4",
          "message": "benchmark: we can't use continuous line for jq input",
          "timestamp": "2024-02-07T08:32:06+09:00",
          "tree_id": "b24e45eba93af22a7e1e3e9bd39b537f39d92b04",
          "url": "https://github.com/groonga/groonga/commit/0fc3552069ef94d1eaf1f19fdef6917d67155dd4"
        },
        "date": 1707262672688,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3660646650000672,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01599999999999918 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2738569790001293,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017010000000000636 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01676009300001624,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00048000000000122944 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01637997399990354,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00046000000000107066 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09609727399998746,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030567999999999873 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07905269099995849,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026633000000000157 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01870638699989513,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002104999999999524 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028005016999998134,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018139999999995382 s\nthreads: undefined"
          }
        ]
      }
    ]
  }
}