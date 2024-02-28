window.BENCHMARK_DATA = {
  "lastUpdate": 1709137276105,
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
          "id": "831ce110ba9f471fb258188710662c9b8ac500ed",
          "message": "benchmark: flatten is still needed",
          "timestamp": "2024-02-07T09:15:22+09:00",
          "tree_id": "8b7af470f51f6061fe230be77b93957bc065ab89",
          "url": "https://github.com/groonga/groonga/commit/831ce110ba9f471fb258188710662c9b8ac500ed"
        },
        "date": 1707265248422,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37026046400023915,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013014999999999971 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2785585839999385,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0166360000000004 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017360129000053348,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000396000000000285 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016491317000088657,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036200000000111143 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2621683709998024,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02531199999999957 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15224628899983372,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0245410000000007 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.009651758999950744,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001878999999999742 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017143093000015597,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018329999999996682 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09658980900019287,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03064799999999976 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07927744599993503,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025537999999995745 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019048353000016505,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002203000000000621 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028373054999917713,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00205600000000003 s\nthreads: undefined"
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
          "id": "6a0a4a88cc9afefdc0e72d2b1ef34185fb37159a",
          "message": "load arrow parallel: add missing error information",
          "timestamp": "2024-02-07T11:12:30+09:00",
          "tree_id": "6f5a05486653c422df5ae8de47c8b9e264abd13a",
          "url": "https://github.com/groonga/groonga/commit/6a0a4a88cc9afefdc0e72d2b1ef34185fb37159a"
        },
        "date": 1707272284291,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3698599639999429,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018277000000000182 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2670479139998747,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01477699999999782 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016816683999991255,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034099999999970265 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.025496500000087963,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004079999999992978 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25599165700003823,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02309800000000002 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14663076000027786,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023141999999998886 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018530598999916492,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018670000000007292 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01750378800005592,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001956999999999931 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0794421970001622,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02429100000000034 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0760188559999051,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024543000000001758 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018484475999969163,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019050000000007117 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018410219000031702,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018980000000004549 s\nthreads: undefined"
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
          "id": "4019feec902d493a1c722056a49f587ba22fc944",
          "message": "Fix style",
          "timestamp": "2024-02-07T11:20:23+09:00",
          "tree_id": "112c0ff973020c76425aafc1fb9f4c3d5cc8d17d",
          "url": "https://github.com/groonga/groonga/commit/4019feec902d493a1c722056a49f587ba22fc944"
        },
        "date": 1707273627456,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38886295799932213,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018689000000000677 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2788069399997539,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017970999999997933 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01712355899985596,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034600000000040154 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016262384000015118,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037299999999998446 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25707365299956564,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02424700000000038 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1480611670003782,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021788999999998865 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.020240000999933727,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0036119999999999763 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017293601000005765,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019079999999993547 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08135583900013899,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02549399999999946 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07639143200003673,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02394700000000133 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01945538900019983,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002154999999999657 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01840346599988152,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001735999999999266 s\nthreads: undefined"
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
          "id": "bce488089c41ef228730d279a0b5b8dd66f59420",
          "message": "filter: use uvector as much as possible for array literal\n\nIf all elements have the same type, we use uvector instead vector.",
          "timestamp": "2024-02-07T16:54:40+09:00",
          "tree_id": "85947ca7d2d41fc38ccf8a2d077096ba000fc607",
          "url": "https://github.com/groonga/groonga/commit/bce488089c41ef228730d279a0b5b8dd66f59420"
        },
        "date": 1707292870822,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3618881800001077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014443999999998486 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26782233099999075,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014039000000001328 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016769214000021293,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003609999999989455 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016213023000034354,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034099999999970265 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25928926299985733,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025649000000000477 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.148083196999778,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02234000000000025 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018454044999941743,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00198400000000018 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017400379999969573,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001876999999998047 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08141122400019185,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024923999999997393 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.078845721000107,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02572200000000116 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019055189000027895,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020069999999994537 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01860446899991075,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018980000000015096 s\nthreads: undefined"
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
          "id": "58057503de92dfa9b91b6446888d88929ccfd90c",
          "message": "distance: fix a bug that validation error message may break memory",
          "timestamp": "2024-02-07T17:22:06+09:00",
          "tree_id": "25c427ab3b612a42194430ca0757bacb4cc24631",
          "url": "https://github.com/groonga/groonga/commit/58057503de92dfa9b91b6446888d88929ccfd90c"
        },
        "date": 1707294443208,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36665673500004914,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017598999999999226 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2773471519998907,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018191000000000013 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017164501000024757,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004460000000001685 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01689494700008254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037200000000048306 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25692621799976223,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024347000000000993 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15183386999984805,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02460100000000054 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018597286999977314,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018369999999999775 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017508835000001,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018729999999979319 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08705807699976731,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027148000000000033 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08360134300005484,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028958999999998652 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01912247200004913,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020969999999988775 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01794552799998428,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017780000000016116 s\nthreads: undefined"
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
          "id": "b79ee105b18ee2d265c44a150a5edbee6c852865",
          "message": "cmake blosc: use 2.13.2",
          "timestamp": "2024-02-08T10:14:21+09:00",
          "tree_id": "38dda0cddd77dd13539efd4764490d5e34d0add0",
          "url": "https://github.com/groonga/groonga/commit/b79ee105b18ee2d265c44a150a5edbee6c852865"
        },
        "date": 1707355188154,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36779638799998793,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014370000000000549 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2773740259996771,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01611200000000157 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016603758000030666,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00033699999999894925 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01631735299997672,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003659999999996444 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2571980260000828,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024424000000000418 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.156540623000069,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022431000000001894 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017871131999925183,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001738999999999713 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017679249000025266,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018929999999995617 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0824318850002328,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025674000000000613 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07507673399982195,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024155999999998068 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018566937000059625,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020890000000012565 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.023830779999968854,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018570000000012465 s\nthreads: undefined"
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
          "id": "c38a214193f901d0654d1f74cb67ace8c351a729",
          "message": "mruby: use \"git submodule\" instead of \"conf.gem github:\"\n\nWe don't need to do \"git clone\" on build time by this.",
          "timestamp": "2024-02-08T22:42:48+09:00",
          "tree_id": "848c6127c5c4441902d9d31365466638ba3d0c7d",
          "url": "https://github.com/groonga/groonga/commit/c38a214193f901d0654d1f74cb67ace8c351a729"
        },
        "date": 1707400170993,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3732895280001003,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01607200000000028 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2721579430001384,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014558999999999295 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016505053000003045,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003750000000009024 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01629851000001281,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000364000000000253 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2584914390001245,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02669399999999908 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15104302399998915,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024504000000000803 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01865559499998426,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016829999999992962 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01740874399996528,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016889999999989969 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08957730899993521,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030074999999998908 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0813565600000743,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028535999999999923 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018991077999942263,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001962000000000408 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018516059000035057,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017520000000003921 s\nthreads: undefined"
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
          "id": "f385baf6f97db2da1ccb444c699d326754b1d2f5",
          "message": "Use bool instead of grn_bool in romaji.c\n\nhttps://github.com/groonga/groonga/issues/1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-09T08:03:54+09:00",
          "tree_id": "0dd6dfc7b89a76055da4741fbe28194e963cbcb5",
          "url": "https://github.com/groonga/groonga/commit/f385baf6f97db2da1ccb444c699d326754b1d2f5"
        },
        "date": 1707434237268,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3605532770003492,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017105999999999705 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27981583400003274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01787700000000106 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017165110000064487,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003340000000004728 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.023661481999965872,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040599999999990644 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2560905440004717,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025051999999999353 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1516324289999602,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023397000000000556 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018694049999965046,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018710000000012328 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017490150999947218,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002015000000000433 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08662610400023141,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028776999999997638 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08184939200020835,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028478999999997534 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018546809000042686,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002001999999999171 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028235667999922498,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019049999999998235 s\nthreads: undefined"
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
          "id": "8911db0d866dfab95a2252474c9a90c2948c98aa",
          "message": "distance: fix ifdef position",
          "timestamp": "2024-02-09T16:38:45+09:00",
          "tree_id": "01aaacc8663de10f82bcc351b08ea17d30ed78b1",
          "url": "https://github.com/groonga/groonga/commit/8911db0d866dfab95a2252474c9a90c2948c98aa"
        },
        "date": 1707464675900,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.356204681999543,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014339000000000823 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27015204900078515,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012986000000000497 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017929170999991584,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004419999999987212 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.0175729349998619,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005379999999997054 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25011848900010136,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03107600000000059 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1603891290002366,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031351999999997646 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01832473099989329,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020860000000000045 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01822548600000573,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019499999999998963 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.10727538299977368,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.035019999999999246 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.09105794000004153,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030149999999998817 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.020176822000053107,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002062999999999343 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02826242399999046,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002025999999999334 s\nthreads: undefined"
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
          "id": "f5d2de0e3488455a1812514f9a402ec1582f86a1",
          "message": "Use bool instead of grn_bool in file_lock.{c,h}\n\nhttps://github.com/groonga/groonga/issues/1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-12T13:54:51+09:00",
          "tree_id": "18ecdbd2a186b3e3e9d5b2715302f4d9bf02e0ca",
          "url": "https://github.com/groonga/groonga/commit/f5d2de0e3488455a1812514f9a402ec1582f86a1"
        },
        "date": 1707714157380,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36159595300000547,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01657100000000014 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27623473200003446,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016161999999999982 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01697564399995599,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035299999999960363 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016236789999936718,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032099999999957163 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.23906081900003073,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02307200000000023 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1466774110000415,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023810000000000747 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01851814399998375,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018069999999990316 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01719621500001267,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016509999999999858 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08448581199996852,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023987999999999884 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07812226599986616,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025055000000001576 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01847505400002092,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001992000000001326 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027576502999977492,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019009999999999583 s\nthreads: undefined"
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
          "id": "0c69b0f76f62fb9a522162a57121d4ce20847f42",
          "message": "Use bool instead of grn_bool in normalizer.c\n\nhttps://github.com/groonga/groonga/issues/1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-13T08:52:07+09:00",
          "tree_id": "1c4b23ca1aa9e1826715c533452447565032bca9",
          "url": "https://github.com/groonga/groonga/commit/0c69b0f76f62fb9a522162a57121d4ce20847f42"
        },
        "date": 1707782543103,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3479921690000083,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0152869999999998 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2663605650000136,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01357999999999987 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01767469299994673,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036399999999936483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016002955999965707,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003269999999999107 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.23779208599995627,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022057999999999328 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14645623699976795,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0230060000000005 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01800604100003511,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017719999999999958 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017915923000032308,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020219999999997462 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08544100499983642,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025339999999997906 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0766708860000449,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02519600000000205 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018804032000105053,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002188000000001189 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018498610999927223,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020240000000000813 s\nthreads: undefined"
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
          "id": "be9e3ec0d4ced73ad3393254fb256953d17cfa5a",
          "message": "Add `n_workers` to output of `status` (#1699)\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-14T12:10:39+09:00",
          "tree_id": "e354c5c6795e99af6957cd97e7b0d6b3c17a4d0c",
          "url": "https://github.com/groonga/groonga/commit/be9e3ec0d4ced73ad3393254fb256953d17cfa5a"
        },
        "date": 1707880615121,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38668563199979644,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021507000000000498 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27594840199998316,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015009999999999885 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016455978000067262,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00041000000000046555 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01632649500010075,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035300000000138 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26598705300000347,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028999999999999707 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15366562200011913,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02661299999999847 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018624616000010974,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018699999999993722 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.018435062999969887,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002397000000000399 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.10287916700008282,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03693300000000041 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08703597499999205,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03079499999999874 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018847143000073174,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001947000000000365 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01932027899994182,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022120000000000473 s\nthreads: undefined"
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
          "id": "bb34ffa313f71777a246e3b346fb432d35d9f84f",
          "message": "cmake simsimd: disable by default again\n\nIt seems that we require C11 with GCC/clang for _Float16.\n\nIt seems that Visual C++ isn't supported.",
          "timestamp": "2024-02-16T06:36:06+09:00",
          "tree_id": "5341bad764c4654adf22a474b0ebb906aaba02e3",
          "url": "https://github.com/groonga/groonga/commit/bb34ffa313f71777a246e3b346fb432d35d9f84f"
        },
        "date": 1708033443518,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3804886829998395,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01567000000000006 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2772662940000714,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015145000000000047 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01727931000004901,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003649999999995046 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016618645999926684,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003690000000000082 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25606837799989535,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022070999999999757 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1477203470001882,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022021000000000263 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018371606000016527,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017630000000001256 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01734090199994398,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017270000000002839 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08184155499975532,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02344499999999909 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07446391300010191,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023121999999996395 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019021807999934026,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020560000000008627 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018528218999961155,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017630000000004031 s\nthreads: undefined"
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
          "id": "85d0a8dbb0a38744aa6051e9a9c6b308ef78ac0f",
          "message": "distance inner-product: use element type\n\nIt improves performance.",
          "timestamp": "2024-02-16T11:24:17+09:00",
          "tree_id": "c800ae246d0e9cdb88a184ade323003a8150fdd4",
          "url": "https://github.com/groonga/groonga/commit/85d0a8dbb0a38744aa6051e9a9c6b308ef78ac0f"
        },
        "date": 1708050634665,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37576313500005654,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018388999999999572 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2769798730002435,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016124999999999085 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.0168737000000192,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034799999999890474 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01652449799991018,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032499999999924256 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26107415199999195,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02560399999999992 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15167986800003064,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02557400000000079 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018815274000019144,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021320000000002726 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01781564499998467,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017889999999978201 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0952808890000938,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02980700000000125 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08301187900030982,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027881000000001294 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019691517000069325,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002054000000000694 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018533433999891713,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001900999999999875 s\nthreads: undefined"
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
          "id": "eba938e04016699e6ab345baedaca3c7b36e2681",
          "message": "distance l1-norm l2-norm-squared: use element type\n\nIt improves performance.",
          "timestamp": "2024-02-16T11:32:10+09:00",
          "tree_id": "71e39a3d784d89a443eabf998833588facdce660",
          "url": "https://github.com/groonga/groonga/commit/eba938e04016699e6ab345baedaca3c7b36e2681"
        },
        "date": 1708051711792,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37858463799989295,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01647299999999885 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2818905489999679,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012254999999997768 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01684325999997327,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003829999999984679 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.025726168999995025,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004019999999993473 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2582527839999784,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0245959999999998 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1506832439996515,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024679999999997676 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018896290999975918,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002053000000000388 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.0175210719999086,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018069999999997532 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09382216599993853,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.032375000000000126 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08428360899989684,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02966300000000094 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01912177200000542,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020039999999998115 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.025949855999954252,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017860000000002318 s\nthreads: undefined"
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
          "id": "f8a685ed5ac94b4e606ecc187aca9eb349376166",
          "message": "grn_obj_is_scalar_accessor: add",
          "timestamp": "2024-02-16T15:48:12+09:00",
          "tree_id": "f46d1d3e4873ebf6c6b9cbb4aa7f088b277b0312",
          "url": "https://github.com/groonga/groonga/commit/f8a685ed5ac94b4e606ecc187aca9eb349376166"
        },
        "date": 1708066438866,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3861618909998583,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019726999999999995 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2661688410001375,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012897999999998966 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017295616000012615,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038400000000038403 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016293666999956713,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003609999999998337 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2618984339999315,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024535999999999655 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1521407910003063,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024406000000000677 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017532714000026317,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016799999999994597 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01716007099997796,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017310000000000103 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0841473430002111,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026901999999998955 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08036682800008066,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028849999999999265 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018789363999985653,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002086000000000393 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01840136999993547,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017730000000002466 s\nthreads: undefined"
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
          "id": "24e683fff15bcf5f636005256c33e61dec88ae17",
          "message": "rset: add missing GRN_BULK_REWIND() for temporary bulk",
          "timestamp": "2024-02-16T18:05:52+09:00",
          "tree_id": "4a3ad9cfb3f9822aa61455882c795d4e42f2ad44",
          "url": "https://github.com/groonga/groonga/commit/24e683fff15bcf5f636005256c33e61dec88ae17"
        },
        "date": 1708074777905,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3726691479998294,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018014000000000127 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27638550400013173,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01634499999999975 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01700794699996777,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005850000000009459 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01620733000004293,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038800000000094315 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2599741399998834,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02570900000000058 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15326411600000256,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025989000000000068 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018547766000040156,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019839999999998192 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017707495999957246,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017910000000004866 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08901142799993522,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02939999999999976 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08074516900001072,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0265669999999978 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018280911999966065,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001893999999998286 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027847609000048124,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019689999999998875 s\nthreads: undefined"
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
          "id": "f2907f328d8e6f5700bbb772fa810c0058ac291d",
          "message": "grn::bulk::put: add",
          "timestamp": "2024-02-16T18:07:24+09:00",
          "tree_id": "969474b457f3042a1dad3c0f3ee56538386192ae",
          "url": "https://github.com/groonga/groonga/commit/f2907f328d8e6f5700bbb772fa810c0058ac291d"
        },
        "date": 1708074936319,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.372117681000077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01729299999999892 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2622505520001255,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011823999999999807 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017036722000057125,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039499999999925706 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.02555337500001542,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038499999999963563 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.28025659200017117,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025470000000000714 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15042646799997783,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023892999999999526 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018423846000075628,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018969999999995935 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017447805999893262,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019850000000003476 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08103515099992364,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026114999999999555 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08020268199987868,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02695299999999967 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01896268900003406,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020520000000013305 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0184423580000157,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001914999999999445 s\nthreads: undefined"
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
          "id": "2a8df0b584afc5a77ccde948fbf4fde8f18c7faa",
          "message": "grn_obj_cast: use grn::bulk::put() not set()\n\nIn general, grn_obj_cast() appends casted data to dest not replaces\ndest.",
          "timestamp": "2024-02-16T18:08:29+09:00",
          "tree_id": "f90ec01cd583e332f811ed32180d3bfe7c76a16c",
          "url": "https://github.com/groonga/groonga/commit/2a8df0b584afc5a77ccde948fbf4fde8f18c7faa"
        },
        "date": 1708075700641,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.35994716300012897,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015956000000000053 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2618052040002681,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012517999999997947 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01662778000002163,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004129999999991085 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016259564999984377,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035400000000102017 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2511179739998397,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02319499999999884 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15133301600008053,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02444099999999952 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01820401400004812,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002005000000000451 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017754315000047427,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019830000000012338 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.07963147599997455,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02504200000000012 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07583203499979163,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02423599999999837 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019276024000021152,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020850000000005586 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027620144999957574,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019050000000015999 s\nthreads: undefined"
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
          "id": "1589c34c7b1422f71ebe59a030cb35a1b052bd7c",
          "message": "grn_obj_cast: add support for numeric uvector",
          "timestamp": "2024-02-16T18:10:14+09:00",
          "tree_id": "b08120ce97530bbfab4ac3d569b9b4e89f62cfea",
          "url": "https://github.com/groonga/groonga/commit/1589c34c7b1422f71ebe59a030cb35a1b052bd7c"
        },
        "date": 1708076546052,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3769584110000892,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02021799999999971 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27497387799996886,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01607199999999978 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01810836200002086,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036100000000072185 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.0162592489999156,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003429999999999822 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2556790499998556,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02530299999999966 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15514454399993838,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02778999999999776 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.015087143999949149,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001770000000001909 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017949695000027077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002019000000001353 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08366028600005393,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02502900000000026 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07900121399995896,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026138000000002548 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01921929499997077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001977999999998842 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02743458399999099,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016959999999997533 s\nthreads: undefined"
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
          "id": "988fa96e190336e24918dfcef6ff43fb24f70e36",
          "message": "distance: set applier",
          "timestamp": "2024-02-16T18:27:40+09:00",
          "tree_id": "f8575d9469bde1a0f25d55e78c9b31ff3f32a6ee",
          "url": "https://github.com/groonga/groonga/commit/988fa96e190336e24918dfcef6ff43fb24f70e36"
        },
        "date": 1708078930415,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3674506250000036,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01721199999999913 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2715567479999095,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016253000000000767 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016457599000034406,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004549999999999832 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.02561502699995799,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003939999999991173 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2592294810002045,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025226 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15275534099987453,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024394000000001304 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018058275999919715,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019639999999997992 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01755601000002116,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001994999999999969 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.082295386999931,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026310000000001263 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0806105899999352,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0263890000000018 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018889438000087466,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0023219999999997964 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018575472999941667,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018020000000000536 s\nthreads: undefined"
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
          "id": "81232f0d1d3c269e513c33ffd72533a3a4bfe4f3",
          "message": "grn_obj_is_locked db: add missing options check",
          "timestamp": "2024-02-18T06:34:17+09:00",
          "tree_id": "3384de7b5794d337c4bc1c28af39941abdbbcb98",
          "url": "https://github.com/groonga/groonga/commit/81232f0d1d3c269e513c33ffd72533a3a4bfe4f3"
        },
        "date": 1708206034812,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3840438789996483,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016570999999999725 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26500043700013975,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011407 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017456138999989435,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004029999999994871 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016177222000067104,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003329999999994726 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26279664200023944,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025028999999999968 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15052258200006463,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02386599999999864 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018801841000026798,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020050000000001733 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.011548005999941324,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017099999999992122 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08471393900003932,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026608000000001714 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07631214099984618,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0250159999999974 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01911834999992834,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002135999999998639 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018413209000016195,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001815999999999901 s\nthreads: undefined"
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
          "id": "c9d8721d1254b9c2f8685985627e43410bfb6b82",
          "message": "select filter bitwise operations: fix a cast bug\n\nIt assumed that grn_obj_cast() override the destination buffer. But\ngrn_obj_cast() must append a casted value to the destination buffer.\n\nIt assumed that result isn't same as x nor y. But result may be the\nsame as x or y. It's for optimization.",
          "timestamp": "2024-02-18T08:33:11+09:00",
          "tree_id": "06bc8925ae55664506a1c0ec1dcd2a5681ff852e",
          "url": "https://github.com/groonga/groonga/commit/c9d8721d1254b9c2f8685985627e43410bfb6b82"
        },
        "date": 1708213136763,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3878994029996079,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02088800000000006 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2705607439997948,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014125000000002469 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016973049000057472,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003900000000012227 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016414937000092777,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00044700000000119644 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2622943990002682,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02436400000000076 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15098335400017504,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02491700000000091 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01773938000008002,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018069999999999753 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017504613000028257,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020119999999996807 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08860179799995649,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030347999999998154 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07913042799987124,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02563800000000116 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019172057000048426,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020039999999994507 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02759142999997266,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017709999999986625 s\nthreads: undefined"
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
          "id": "fcacd90ed3cb2eb12cfbfb1547c600f3eb992fa0",
          "message": "distance: unify common code",
          "timestamp": "2024-02-18T20:59:39+09:00",
          "tree_id": "18ae9f4e99f5cb1132de85e001f880f472b88f72",
          "url": "https://github.com/groonga/groonga/commit/fcacd90ed3cb2eb12cfbfb1547c600f3eb992fa0"
        },
        "date": 1708257887513,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38947627099997817,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02080600000000002 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2651831550000452,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013336000000002568 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016783763000091767,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004129999999999967 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016245976999925915,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003809999999999647 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2678241450000769,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03108899999999999 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15515701899994383,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02763700000000005 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018992834000073344,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019470000000002263 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017503109000017503,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020380000000008724 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.10242064999988543,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03938600000000088 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08235363800019968,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028653000000004633 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018674197999985154,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020310000000000328 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01877834999999095,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017650000000006827 s\nthreads: undefined"
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
          "id": "1d867b836d88e38b0c84e4333ef09757f9e521db",
          "message": "TaskExecutor::get_n_workers: add",
          "timestamp": "2024-02-18T21:47:50+09:00",
          "tree_id": "1adc93bdb2544787eb21585922b9e9fe4a6e4a30",
          "url": "https://github.com/groonga/groonga/commit/1d867b836d88e38b0c84e4333ef09757f9e521db"
        },
        "date": 1708260788562,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37190900100006274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01301300000000015 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2801692029999572,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015748000000001705 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017259891999970023,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0006099999999990002 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016286238000020603,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003789999999997684 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2601241229996276,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024531999999999693 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15194622200004915,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024469000000000074 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01906359699995619,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021489999999992904 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01743312100006733,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018010000000003856 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08478169800008573,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02584399999999948 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07624810500021795,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025016000000000593 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018690419999984442,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020089999999992614 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0262264430000414,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019079999999996322 s\nthreads: undefined"
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
          "id": "9e132366043ff0500f53ec62d62db1ca8fceac42",
          "message": "applier distance: add support for parallel processing",
          "timestamp": "2024-02-18T21:48:15+09:00",
          "tree_id": "3369a6faf05397064d80306f7549b22a8f4b003e",
          "url": "https://github.com/groonga/groonga/commit/9e132366043ff0500f53ec62d62db1ca8fceac42"
        },
        "date": 1708261241417,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36552655099984577,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01338000000000164 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27135236499970006,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.010872000000000381 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016893285999969976,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037100000000073186 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01610890299997436,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003709999999991498 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25904459699961535,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024519000000000388 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15632914699995126,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022385000000001015 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01797698899997613,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017459999999987763 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017383512999913364,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016480000000003159 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08026873900001874,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024187999999999155 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07457355399992593,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02332499999999843 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01890861100002894,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020459999999991874 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02774184699990201,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001862999999999615 s\nthreads: undefined"
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
          "id": "4807d547862a45ea6bb65abf594b1b7cabb12c90",
          "message": "Use bool instead of grn_bool in proc_schema.c\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-19T09:00:34+09:00",
          "tree_id": "3ba3a580e2b5d1243a319abf2b6cd6db9b54a08d",
          "url": "https://github.com/groonga/groonga/commit/4807d547862a45ea6bb65abf594b1b7cabb12c90"
        },
        "date": 1708301258845,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37542255100009925,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019585000000001018 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2747113599999693,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014370999999997636 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016939968000031058,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005089999999993433 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01639578200001779,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036700000000017274 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2578993499997182,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02431799999999909 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15990245399984815,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02382700000000082 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01826041900000064,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016569999999997975 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01740665500005889,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001825000000000243 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08744724800004633,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02799099999999949 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07942246599986902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027452000000004945 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.012527597999905993,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020119999999997917 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01850662999999031,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017559999999985365 s\nthreads: undefined"
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
          "id": "18060b64b8abd36fb05af474140edf9f01f53557",
          "message": "db: extract grn_db_wal_recover()\n\nWe want to use C++ for it so that we can use grn::TaskExecutor.",
          "timestamp": "2024-02-19T14:09:05+09:00",
          "tree_id": "bcf38e68ff545b02f3419823d8cd0f0bf6267ba5",
          "url": "https://github.com/groonga/groonga/commit/18060b64b8abd36fb05af474140edf9f01f53557"
        },
        "date": 1708319692093,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36198044299976573,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015622000000000691 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2779649009999048,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017055999999999016 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017211727000074006,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00043200000000087613 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016313338999964344,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004510000000007841 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2553670610001859,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02573199999999956 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15427206199973398,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025286999999999893 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.021450399999991987,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.004559999999998621 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01963899700001548,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002036000000000232 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08680852700001651,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029343999999999815 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08184919599972318,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028691000000000882 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018657637999979215,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019649999999984957 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01868691400005673,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017590000000002604 s\nthreads: undefined"
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
          "id": "76367d827eae045ee5221559693ae8211831e6b4",
          "message": "db: make grn_id_map thread safe",
          "timestamp": "2024-02-19T14:42:03+09:00",
          "tree_id": "dfa19067f93e36c250a882894dc37a4d8d4d4cd3",
          "url": "https://github.com/groonga/groonga/commit/76367d827eae045ee5221559693ae8211831e6b4"
        },
        "date": 1708321671281,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36363352900019663,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01618100000000129 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26558619199983013,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013661000000002421 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016750795999939783,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003489999999990445 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016403764000017418,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003339999999996124 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2587294630001793,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02394500000000059 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15121787800023867,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02317999999999651 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01839369499998611,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017349999999999866 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017690919999949983,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018079999999995322 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0826404889998571,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02470000000000039 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08069912300010174,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024836000000001912 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019725417000017842,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021729999999990923 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018252099000051203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001664999999999195 s\nthreads: undefined"
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
          "id": "f777530211d71e5d2183bedcdbca6866d3334baf",
          "message": "distance applier: do nothing for empty target table",
          "timestamp": "2024-02-19T15:14:04+09:00",
          "tree_id": "61d0af3b9136a7e5dad21a59e2b63c27fa3207c3",
          "url": "https://github.com/groonga/groonga/commit/f777530211d71e5d2183bedcdbca6866d3334baf"
        },
        "date": 1708323548711,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3748124589999975,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02056199999999997 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28059152399987397,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01819199999999921 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01706875600012836,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003260000000011587 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01621604799998977,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034800000000001496 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2524679329998776,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025095999999999535 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15150092000016002,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02496000000000173 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018205453999996735,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018999999999987915 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01729577999998355,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017390000000017392 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0827127830000336,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02627200000000135 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0766993220000245,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025504000000003274 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019725583999957053,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021559999999984925 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018685127999958695,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018669999999994802 s\nthreads: undefined"
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
          "id": "293f2cc948e675d80c2096aa508a4b6b3b887d6a",
          "message": "distance applier: don't run needless workers",
          "timestamp": "2024-02-19T15:30:40+09:00",
          "tree_id": "59eff9a8985baad310acbee13601a83b9a279769",
          "url": "https://github.com/groonga/groonga/commit/293f2cc948e675d80c2096aa508a4b6b3b887d6a"
        },
        "date": 1708324718425,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3636062029999607,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016320999999999475 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26194751099984614,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013020000000000254 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01695176500004436,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003979999999996764 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01636298099998612,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000324000000000102 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25278132400006825,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025066999999999978 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15370598399982782,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025407000000002233 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01887486900005797,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019930000000006887 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01781566900007192,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018500000000001293 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08959524699997701,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02967000000000028 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07960465000030581,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026448000000001137 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018937573000016528,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002155000000000157 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018782913000052304,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019199999999985895 s\nthreads: undefined"
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
          "id": "55facb626fe5b26d7b32a7044aae5bf8b013e0d7",
          "message": "distance applier: avoid out of range access",
          "timestamp": "2024-02-19T16:26:05+09:00",
          "tree_id": "0951eae6ab5ea75d798cbc625e6b4e616fed4fb6",
          "url": "https://github.com/groonga/groonga/commit/55facb626fe5b26d7b32a7044aae5bf8b013e0d7"
        },
        "date": 1708327892509,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.368535855000232,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015426999999999857 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2642228750000868,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012275999999999176 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01646168300004547,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040700000000001846 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016013390000011896,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032799999999999496 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26160486200035393,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0259579999999989 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15177676399991924,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02505199999999916 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.020865539999988414,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.004434000000000632 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017333969999924648,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001905000000000101 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08720470799988789,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030519999999999436 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0821760980001045,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029466999999997828 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018476901999918027,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001878000000001323 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02785973499993588,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001818999999999349 s\nthreads: undefined"
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
          "id": "7702d7a99669306b183a59ac1bcf14219c6bbcb5",
          "message": "db wal recover: add support for rebuilding broken indexes in parallel",
          "timestamp": "2024-02-19T17:17:00+09:00",
          "tree_id": "66bcf095fc9201f73f2e81157a666c25658c18b8",
          "url": "https://github.com/groonga/groonga/commit/7702d7a99669306b183a59ac1bcf14219c6bbcb5"
        },
        "date": 1708330980157,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37510641699992675,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02101899999999983 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2756102889999852,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016738999999997978 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01727011599996331,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00042199999999859017 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024100037000039265,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00047999999999959186 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2558136070001069,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02592099999999964 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15310896000016783,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02554399999999915 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018915079999999307,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002161999999998443 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01813000599992165,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019349999999998257 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08979376300027297,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031028000000001305 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07927089100007834,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025082999999998995 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019504958000027273,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002142999999999534 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02840369899996631,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001960000000000406 s\nthreads: undefined"
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
          "id": "3e75a14c91906d768511dc7f253238dfc860f0a0",
          "message": "arrow: Support `Int64` in `output_type=apache-arrow` for columns that reference other table (#1705)\n\nGitHub: fix GH-1704\r\n\r\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-19T18:17:36+09:00",
          "tree_id": "cffac264c3d56ada1b9c4bfa928dbeefd0e28a8f",
          "url": "https://github.com/groonga/groonga/commit/3e75a14c91906d768511dc7f253238dfc860f0a0"
        },
        "date": 1708377058442,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3770406399998478,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017361000000001472 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27879968099995267,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01696899999999875 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017045361000043613,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004949999999999122 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016798954999956095,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00042199999999947835 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5956723130000228,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019300000000052608 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25722063599971534,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024853999999999335 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15823623800002906,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031496999999996916 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018680183000071793,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018209999999985738 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017651258999990205,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016779999999982642 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09587191700029507,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03317799999999868 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08295828900014612,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028135000000002047 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019333327000026657,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021229999999992366 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028126244999953087,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017680000000002138 s\nthreads: undefined"
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
          "id": "a7a7c8d690823112984c09dec643555209b6611d",
          "message": "Use bool instead of grn_bool in proc_object.c\n\nhttps://github.com/groonga/groonga/issues/1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-21T08:09:41+09:00",
          "tree_id": "768a38e00989856660458e4a9c06c52c6c813e1f",
          "url": "https://github.com/groonga/groonga/commit/a7a7c8d690823112984c09dec643555209b6611d"
        },
        "date": 1708471047625,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3723450970001636,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018627000000001004 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26563168700016604,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01366399999999851 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01748502800001006,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005050000000000887 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01669597599999406,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039200000000150226 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6679900080000039,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00015700000000007375 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2627919420000353,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02764699999999949 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15393650100003242,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025945999999999872 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01851200800007291,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018109999999999238 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017836746000000403,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018649999999986178 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0867639190000773,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02787300000000055 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08075241599988203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028290000000003868 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01919868899994981,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021290000000009357 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028191505999984656,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017589999999991224 s\nthreads: undefined"
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
          "id": "20297a2235fe4cee30ec70742a4f313dc3b05224",
          "message": "Use bool instead of grn_bool in proc_dump.c\n\nGH-1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-22T08:11:51+09:00",
          "tree_id": "31c6f13a64b1c180957b626b5c443a76e7d1daba",
          "url": "https://github.com/groonga/groonga/commit/20297a2235fe4cee30ec70742a4f313dc3b05224"
        },
        "date": 1708557589636,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3814320369997972,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021423000000000733 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2818007390000048,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017857000000001344 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016788522000013018,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003749999999993203 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016591272999960438,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003740000000007626 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.517256572000008,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018100000000048633 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25604125100005604,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02467499999999974 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1499550079998926,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024061 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01851733799998101,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001845999999998682 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017218756000090707,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016799999999994597 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0869605729998284,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029387999999998943 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07978779200027475,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028679999999998013 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019884076000039386,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002166000000001611 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028402628999970148,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018069999999991704 s\nthreads: undefined"
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
          "id": "1af459f59514e76065a56c13dc23866c1366cb86",
          "message": "ii: fix printf formats",
          "timestamp": "2024-02-22T10:16:45+09:00",
          "tree_id": "145218d8a1bbef20b1d171087500f38cd0feca6b",
          "url": "https://github.com/groonga/groonga/commit/1af459f59514e76065a56c13dc23866c1366cb86"
        },
        "date": 1708564935047,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3810278700000822,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021543999999999536 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26194467000016175,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012484999999999635 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016681360999939443,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036200000000002897 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016452709999953186,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036699999999978417 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4297321070000066,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020000000000003348 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2565605940002911,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02539399999999996 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15088527199969803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02432400000000108 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018402672000092934,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018259999999994392 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017950104000021838,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020340000000000913 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08342681500016624,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026551000000001462 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07862085099981186,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025802000000003517 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01866337699993892,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020460000000008804 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018530607000059263,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017940000000002954 s\nthreads: undefined"
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
          "id": "2eed4188cdd7b5db56cd455384344812b1b94cd5",
          "message": "Use bool instead of grn_bool in hash.{c,h}\n\nGH-1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-26T08:02:36+09:00",
          "tree_id": "7e9c89e5b7059c4a0e22cc5b084df3a257c6f204",
          "url": "https://github.com/groonga/groonga/commit/2eed4188cdd7b5db56cd455384344812b1b94cd5"
        },
        "date": 1708902861988,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36803548199986835,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017267999999999784 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26910653800018736,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014085999999997156 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016563993999966442,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003509999999993241 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016245113999900695,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00033599999999989194 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5412580029999958,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021700000000018926 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25721885499984865,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02636999999999924 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15263727500013147,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024570999999997456 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01873716199992259,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019239999999999813 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017465458000060607,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018320000000004721 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08734558000054449,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03022100000000029 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08174802400014869,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0286789999999974 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019736780999949133,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020880000000013665 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02823990000001686,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001953999999999928 s\nthreads: undefined"
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
          "id": "bf353bfbcb5ba0c1646d79cb3bfea01d1f1cdb5c",
          "message": "Use bool instead of grn_bool in proc_table.c\n\nGH-1638",
          "timestamp": "2024-02-27T08:35:38+09:00",
          "tree_id": "0391774ea6f7c8e15a6af19f6069cc159c97731d",
          "url": "https://github.com/groonga/groonga/commit/bf353bfbcb5ba0c1646d79cb3bfea01d1f1cdb5c"
        },
        "date": 1708991079732,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37784774999988713,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020076000000001593 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2682386859997905,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013308999999999654 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017295003999947767,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035799999999941434 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01643385799997077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000386999999999027 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3948492089999718,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022100000000074838 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2558925710000608,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024385999999999547 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15025540300007378,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022727000000001496 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018447455000000446,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017520000000000036 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01735945700005459,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00171300000000002 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08420582499968532,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026220999999999772 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07722412500004339,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025696999999999248 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01915902600001118,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002123000000000097 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018422509000004084,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016970000000000596 s\nthreads: undefined"
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
          "id": "2f592a635036a6d089b61b14788047cd2a391052",
          "message": "clang-format: add include/groonga/tokenizer.h",
          "timestamp": "2024-02-27T17:09:55+09:00",
          "tree_id": "626fc0b68d535b86de31597b0f928cf7e97f510a",
          "url": "https://github.com/groonga/groonga/commit/2f592a635036a6d089b61b14788047cd2a391052"
        },
        "date": 1709021769283,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37102349999975104,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01782300000000045 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26223464599962654,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01192499999999716 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01712782000009838,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003279999999996619 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01608244700008754,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00031899999999929207 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3512064150000356,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021999999999972042 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25016211700017266,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022906999999999525 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14559018999989348,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02258200000000124 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017429521999986264,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016389999999990579 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017191895999928875,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001699000000001838 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08252974600014795,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026002999999999526 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07432169800011934,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0238199999999969 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018252879999977267,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019520000000004256 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018057005999935427,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018100000000007554 s\nthreads: undefined"
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
          "id": "abea9de74fd06b52fbafb4af2f6f516ac2a935ca",
          "message": "clang-format: add lib/grn_tokenizer.h",
          "timestamp": "2024-02-27T17:10:44+09:00",
          "tree_id": "ba93d4dbaa74eeeb5ff6b028002fa3e95260b01f",
          "url": "https://github.com/groonga/groonga/commit/abea9de74fd06b52fbafb4af2f6f516ac2a935ca"
        },
        "date": 1709021824384,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3777382739999098,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017247000000000193 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26304261899991843,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012568000000000523 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01713670800000955,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003520000000012402 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.025975585999901796,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003969999999978713 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4508871780000163,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022099999999955489 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2551582089997737,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02343500000000015 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14990286600027503,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02315399999999955 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.02334418000009464,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002124000000001125 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017407258000048387,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001798999999999884 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08332646300010538,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025657000000000582 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07787043299970264,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02507599999999982 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01893494799992368,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019010000000000138 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018506461000015406,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018979999999997332 s\nthreads: undefined"
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
          "id": "22ba8216fbf4098894dcc1b0c0cd60f4a485363c",
          "message": "Fix order",
          "timestamp": "2024-02-27T17:11:26+09:00",
          "tree_id": "8290cee647525826f7751457471cc4ac597ac127",
          "url": "https://github.com/groonga/groonga/commit/22ba8216fbf4098894dcc1b0c0cd60f4a485363c"
        },
        "date": 1709022533302,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37437931999977536,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01733400000000017 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2688433010000608,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014777999999998293 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01722727199990004,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003549999999998832 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.02587499099990964,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00043199999999998795 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3695375779999495,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021799999999944086 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25692828700027803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02505000000000107 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14935108400015906,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023828999999999795 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018385435999903166,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018150000000014543 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017647387999886632,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018540000000007717 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08376086600037524,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02778600000000002 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07641747100024077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024976999999999194 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01821310699983769,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001971999999999141 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.024207454999782385,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016859999999994102 s\nthreads: undefined"
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
          "id": "21a1fa60ae7e021dcb686d10d0cc0a69a0991e3a",
          "message": "tokenizer: add grn_tokenizer_query_get_data()\n\nInternal grn_tokenizer_query_set_data() is also added.\n\nWe can add data domain information now. It means that we can accept\nnon text value as query. For example, we will accept WGS84GeoPoint as\nquery.",
          "timestamp": "2024-02-27T17:12:13+09:00",
          "tree_id": "c05455c4ebf8673c8c71758c1d0a30c223b3373b",
          "url": "https://github.com/groonga/groonga/commit/21a1fa60ae7e021dcb686d10d0cc0a69a0991e3a"
        },
        "date": 1709023058083,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38589580900014653,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018130000000000687 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2911605769995731,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01812600000000117 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017523042999982863,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004240000000006461 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01619059099994047,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036399999999936483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4752858999999603,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.000365000000000143 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25314414000007446,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02431699999999974 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.159068340000033,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023399999999999685 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01783914399987907,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018870000000004439 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017405080999935763,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017020000000008695 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08564312600015,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02906899999999861 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07796422399991343,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025559000000004217 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01850446900004954,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020230000000012183 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02817788999993809,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016860000000002984 s\nthreads: undefined"
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
          "id": "394045dead42c091bc4d977e9100ba1c61d484b0",
          "message": "token: add grn_token_{get,set}_domain()\n\nWe can add data domain information to token now. It means that we can\ngenerate non text value as token. For example, we will generate UInt64\nvalue as query.",
          "timestamp": "2024-02-27T17:15:54+09:00",
          "tree_id": "4e488db147c996622de35d857ca62df287a09dbd",
          "url": "https://github.com/groonga/groonga/commit/394045dead42c091bc4d977e9100ba1c61d484b0"
        },
        "date": 1709024079575,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36601236199993537,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015678999999999915 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2736229649997881,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013027000000001468 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016603876000090168,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037700000000029377 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016342145000010078,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004139999999992483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3797598010000343,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020399999999981544 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.255851031000077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023638999999998855 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15978873000005933,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023232000000001113 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01795081999989634,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017819999999995062 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017291074000013396,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019650000000011048 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08493238599999131,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027258999999999797 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0754138490001992,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025781999999998306 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018581305999987308,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020150000000001 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02763539100004664,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001907000000000103 s\nthreads: undefined"
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
          "id": "41d59a36922093b53fac114ff365a094ba71639c",
          "message": "clang-format: add include/groonga/token.h",
          "timestamp": "2024-02-27T17:18:54+09:00",
          "tree_id": "bebb544639f87aea8d88173f40432aa89ed22c3e",
          "url": "https://github.com/groonga/groonga/commit/41d59a36922093b53fac114ff365a094ba71639c"
        },
        "date": 1709025623464,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38318962400012424,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022197000000000244 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2724060480000503,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012649999999999745 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01760070400001723,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003919999999988377 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016210442999977204,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003429999999999822 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4157248320000235,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00024400000000010524 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2611021329997243,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02638900000000137 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1622771389996842,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02452600000000052 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017708937999998398,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018990000000000118 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01772654999996348,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001962999999999604 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09547175000000152,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03394600000000013 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08015207300002203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028081000000002132 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018629076000024725,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020080000000000098 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02800878600004353,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018199999999986005 s\nthreads: undefined"
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
          "id": "914d3b430cc954d92588acbdea03f1db1ba4e887",
          "message": "clang-format: add lib/token.c",
          "timestamp": "2024-02-27T17:19:24+09:00",
          "tree_id": "3f00c5b11bd2dbeceb41fe98cf2ab64ce12d3000",
          "url": "https://github.com/groonga/groonga/commit/914d3b430cc954d92588acbdea03f1db1ba4e887"
        },
        "date": 1709026121566,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3589612950000287,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01580699999999871 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2737788699998305,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01709700000000061 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01631269900008192,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035400000000065934 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016036116000009315,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003609999999980573 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3244177659999536,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020799999999954188 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24621211600015158,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02245299999999953 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15448207300022432,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022134999999998975 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018498083000054066,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016599999999994397 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016095398000061323,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016599999999997173 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.079378505000534,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024360999999999078 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0736367860001792,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02252800000000188 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01764905400000316,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017969999999999375 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017467936999992162,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0015960000000005414 s\nthreads: undefined"
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
          "id": "b4b2a2bb66aa9635a8043622094020fc839412e9",
          "message": "clang-format: add lib/proc/proc_tokenize.c",
          "timestamp": "2024-02-27T17:20:56+09:00",
          "tree_id": "59a1ae328e31d2bd6c6fff5953bc7aa8408ce0fc",
          "url": "https://github.com/groonga/groonga/commit/b4b2a2bb66aa9635a8043622094020fc839412e9"
        },
        "date": 1709027010575,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3836137480001298,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015135000000000093 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2714598210000645,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011812000000000517 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016927232999989883,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003760000000000152 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016216981000013675,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037299999999967914 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3636917079999762,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018600000000015826 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25845237099986207,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023089000000000193 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15848597000001519,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023343000000000613 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01809324799995693,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019530000000012038 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017796715000031327,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019599999999986295 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0848179170000094,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027285000000000004 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07968046200028311,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027006000000003 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01880103700005975,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002086999999999145 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026197348999914993,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018809999999998273 s\nthreads: undefined"
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
          "id": "1480d67c044d9eb1388be9a4c33e064d5d5989f6",
          "message": "grn_tokenizer_query: restore ABI compatibility\n\nIt's broken by 21a1fa60ae7e021dcb686d10d0cc0a69a0991e3a accidentally.",
          "timestamp": "2024-02-27T19:33:44+09:00",
          "tree_id": "740743a660a7aa2567faa34612e2e80f952ddd66",
          "url": "https://github.com/groonga/groonga/commit/1480d67c044d9eb1388be9a4c33e064d5d5989f6"
        },
        "date": 1709038743703,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3851262909999491,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019346000000000224 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2877968660000647,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01814299999999866 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015477160000045842,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003319999999996659 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01653519599989295,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003420000000000645 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4427977760000203,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0001949999999996399 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2709725530002629,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025367000000000445 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15990039800021805,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022399999999998088 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016927815999963514,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017119999999997415 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01684828700001617,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018910000000000038 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09638962099995751,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03593400000000149 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0844929870002602,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030359000000001413 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017736205999938193,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002123000000000097 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027035511000008228,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019690000000007757 s\nthreads: undefined"
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
          "id": "32d5e35ddb326c77665ebaf31410f8b976e51be9",
          "message": "Remove unnecessary code related to queue support  (#1713)\n\nhttps://github.com/groonga/groonga/commit/e1e00de51aef3705830d835194dbda4825898817\r\n\r\nIt seems that the related code was removed in that commit.\r\n\r\n---------\r\n\r\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-28T09:21:10+09:00",
          "tree_id": "35ae29b11283a5836f8393e88b378c657a59bb80",
          "url": "https://github.com/groonga/groonga/commit/32d5e35ddb326c77665ebaf31410f8b976e51be9"
        },
        "date": 1709080478010,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.362514610000062,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013507000000000435 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.275478932000226,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01535899999999829 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016608356000006097,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040700000000015724 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016606220999960897,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034299999999826136 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5308424840000043,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019299999999949913 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.258380460000069,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027658999999999753 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15571637799990867,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027900000000000175 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01559907299997576,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002057000000000836 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017736461999959374,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00202700000000014 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09126732299989726,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031838999999999895 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08242675700017799,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02839399999999853 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018456654000033268,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020690000000005426 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.03486178199995038,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002728000000001174 s\nthreads: undefined"
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
          "id": "6b0c20f8bc4fa95fd1c7de88324fdd0271ed65d7",
          "message": "Add support for building with H3",
          "timestamp": "2024-02-28T09:32:27+09:00",
          "tree_id": "b7f9e1fd05e08cd14a3d1998bf759d4be72c16d2",
          "url": "https://github.com/groonga/groonga/commit/6b0c20f8bc4fa95fd1c7de88324fdd0271ed65d7"
        },
        "date": 1709083893905,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.378441559999942,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02033500000000049 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2675173069997072,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013588999999999102 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016465133000110654,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035699999999927456 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01809860799994567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003799999999998249 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3757911959999092,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020600000000042806 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2570968649997667,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024700000000000014 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15029229999993277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023672999999998473 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.019051074999993034,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020170000000000188 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01772784300010244,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002042999999999573 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0801556800004164,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026294999999998875 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07690740400039431,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0263340000000003 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019602683000130128,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021209999999999285 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028210801999875912,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019009999999999583 s\nthreads: undefined"
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
          "id": "39a1e8d514a5873a489917d634485af0b80d2e1e",
          "message": "clang-format: add lib/grn_tokenizers.h",
          "timestamp": "2024-02-28T09:33:45+09:00",
          "tree_id": "5f1124e0c541c314d1e5a3f4b9b50409bf640a70",
          "url": "https://github.com/groonga/groonga/commit/39a1e8d514a5873a489917d634485af0b80d2e1e"
        },
        "date": 1709084228659,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3636066259999211,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016891999999999907 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26217910399969924,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011873999999998275 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.017538408000007166,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004029999999985989 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016179243999999926,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003290000000010229 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4393371250000087,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021999999999955389 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25755598600039775,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026221000000000078 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15117751200017437,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025225000000001108 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01893608700004279,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018950000000002853 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01944415899993146,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018340000000013346 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0859806899997011,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02868799999999877 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08020408399994494,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026130999999998822 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019008207000013044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020230000000000248 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018601523000029374,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001940000000000025 s\nthreads: undefined"
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
          "id": "0b180285b22d4a1d0e01e19026a30c547715e551",
          "message": "TokenH3Index: add\n\nIt tokenizes WGS84GetPoint to UInt64 (H3 index).",
          "timestamp": "2024-02-28T14:00:06+09:00",
          "tree_id": "ce84eaf388f66fab948003ccf050458733b308de",
          "url": "https://github.com/groonga/groonga/commit/0b180285b22d4a1d0e01e19026a30c547715e551"
        },
        "date": 1709096867985,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36722472199926415,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017497999999999195 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2786128170005213,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017708999999999253 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015695305000008375,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034400000000101016 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015492628999936642,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00044200000000149675 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4305611949999957,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021499999999988195 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.254017842999815,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02449500000000049 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1528935610000417,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02526299999999987 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.0172372520000863,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018630000000010583 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017113311000002795,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018080000000011143 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08475583300025846,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02938499999999948 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07653210600028615,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025709999999999678 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017702088999953958,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021460000000006474 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017713981000042622,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019299999999984607 s\nthreads: undefined"
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
          "id": "d1ffa0d9eccaaa6739907e6e9d678fd7fb0881ba",
          "message": "clang-format: add lib/tokenizers.c",
          "timestamp": "2024-02-28T17:15:44+09:00",
          "tree_id": "b3ebd073060884b0d50018bc859f9094cc32f7b4",
          "url": "https://github.com/groonga/groonga/commit/d1ffa0d9eccaaa6739907e6e9d678fd7fb0881ba"
        },
        "date": 1709108605271,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37587323300010667,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016849000000000128 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2706732760001387,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014319999999999777 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01738425799999277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004409999999994696 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016511687999923197,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037600000000087563 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4433298090000335,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00015299999999973668 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2550408270000162,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02512500000000012 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1533829729997933,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026501000000001274 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018751376000182063,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001973999999999476 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017262090999793145,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016949999999998633 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08471481800006586,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02889999999999779 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07921097100029328,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02626699999999932 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018827457999918806,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019200000000001438 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0289237930002173,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002111000000000196 s\nthreads: undefined"
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
          "id": "4f41466b09eb744fcfcad0fac93d75a7f274fb9a",
          "message": "grn_table_add: set correct domain of new key value for index update",
          "timestamp": "2024-02-28T20:03:48+09:00",
          "tree_id": "340af2953164f788a9f1e894e05b414574c2e475",
          "url": "https://github.com/groonga/groonga/commit/4f41466b09eb744fcfcad0fac93d75a7f274fb9a"
        },
        "date": 1709118809804,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37974125599987474,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01985600000000047 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2644982900002333,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011926999999999827 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01572030999989238,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037700000000029377 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015404028000034486,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032500000000013074 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.396653395000044,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00024199999999996447 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25551287999951455,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024071000000001397 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1501484599999685,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023896999999999835 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01731558500000574,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019299999999998207 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01950698099994952,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020279999999997245 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0882432879998305,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02801899999999978 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07929027900047458,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025071000000001065 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018135898999958044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019340000000020452 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027293165999822122,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017690000000012418 s\nthreads: undefined"
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
          "id": "2f780417879568883587487cdd3b85c953d65deb",
          "message": "TokenH3Index: show query domain on error",
          "timestamp": "2024-02-28T20:22:48+09:00",
          "tree_id": "2b72eb7d5e329aa0bc7b178d2900c61ed3865435",
          "url": "https://github.com/groonga/groonga/commit/2f780417879568883587487cdd3b85c953d65deb"
        },
        "date": 1709119732957,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3636782619996666,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017270000000001562 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2619611740000778,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012770000000003362 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01582357100005538,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035600000000002296 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015350790999946184,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004280000000000117 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5478804319999995,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.000244000000000133 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.258610942999951,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028254999999999975 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15359297899999547,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025875999999998484 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01767731500001446,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018829999999999403 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01674511200002371,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019460000000000033 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09108477599988873,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.032189999999999844 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08734385600013184,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03285099999999744 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018614097000011043,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020520000000001926 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0253537140000617,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019299999999997652 s\nthreads: undefined"
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
          "id": "c5af88978689b1a9547fadbdb7489e715d81097a",
          "message": "grn_token_cursor: add grn_token_cursor_set_query_domain()\n\nWe can set domain of tokenized target.",
          "timestamp": "2024-02-28T20:34:15+09:00",
          "tree_id": "fa2247fde29119e50f486eec1ef8e72b2d1c4d4b",
          "url": "https://github.com/groonga/groonga/commit/c5af88978689b1a9547fadbdb7489e715d81097a"
        },
        "date": 1709121859866,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3876337219999755,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02113200000000079 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28159979899999144,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018090000000000078 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015857572000015807,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000370000000000148 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015475431000027129,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003409999999999247 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.480099105000022,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019000000000085615 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.256274043000019,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0246589999999985 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15276474699993514,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0233689999999991 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017080312000018694,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017780000000003349 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017708502999937537,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019289999999997087 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.10865730399979157,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03360900000000018 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.11857246200003146,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027672000000000474 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017275577999953384,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001849000000001766 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02753645999996479,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018750000000009037 s\nthreads: undefined"
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
          "id": "878c3a57b7d68a39c77dbd0e9c3da79769ad6c14",
          "message": "clang-format: add include/groonga/token_cursor.h",
          "timestamp": "2024-02-28T23:17:35+09:00",
          "tree_id": "2b84ee2abdb482bc1edbf4225a2238907e29faea",
          "url": "https://github.com/groonga/groonga/commit/878c3a57b7d68a39c77dbd0e9c3da79769ad6c14"
        },
        "date": 1709130243877,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37211720999982845,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01461499999999985 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2630098369997995,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01083699999999907 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015478769000026205,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036399999999997545 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015419297999983428,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003570000000010509 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3401504130000035,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021700000000005049 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25910104900015085,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02220999999999987 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15098964599980036,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02251599999999976 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01724088399998891,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019479999999996167 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017347476999987066,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018280000000009955 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08447862100001657,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024911999999999657 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0754892759998711,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023104000000000152 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01786343399993484,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020369999999996224 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026913643999989745,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001806000000000002 s\nthreads: undefined"
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
          "id": "6ad45b1bb8eeb605ba1be114835d0810a51e778f",
          "message": "grn_token_cursor: add missing grn_token_cursor_set_query_domain() impl",
          "timestamp": "2024-02-28T23:19:07+09:00",
          "tree_id": "60ef5c57967b03aeae48831b1a6f40dfaea360d8",
          "url": "https://github.com/groonga/groonga/commit/6ad45b1bb8eeb605ba1be114835d0810a51e778f"
        },
        "date": 1709131533711,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3734865379997814,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01903599999999958 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2739027460000898,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015439999999999954 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01563129599998092,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003549999999998832 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01530497600003855,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003700000000012027 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5595900569999799,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002510000000000012 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25214799400009724,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025105000000000877 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15383866700000226,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025880000000002318 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017791435000049205,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018659999999988686 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016915871999970022,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017399999999998528 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09177851900022915,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.032135000000001315 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0833728999999721,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028881000000003404 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01762185600000521,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020259999999998335 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01746514000001298,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017419999999994662 s\nthreads: undefined"
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
          "id": "657b7e3307a368761aa514dedfea272518f57b72",
          "message": "clang-format: add lib/token_cursor.c",
          "timestamp": "2024-02-28T23:19:41+09:00",
          "tree_id": "cdfb193c1dd25cb1eb925faef67f070382b62518",
          "url": "https://github.com/groonga/groonga/commit/657b7e3307a368761aa514dedfea272518f57b72"
        },
        "date": 1709132148664,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3797109739997495,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023020999999999264 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2636533809998127,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012855999999997897 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015716261999898506,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004000000000017323 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.02492138000002342,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003869999999999152 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5888008789999617,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023400000000001198 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25428268999996817,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02960600000000048 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15454641699960803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02636799999999892 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01777154699999528,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020990000000000453 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01749551900007873,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0024140000000008044 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0969804270002328,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03617299999999957 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08200580199974183,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02776099999999901 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018261396000070818,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022300000000005094 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.028250033999995594,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022710000000012442 s\nthreads: undefined"
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
          "id": "d0fdd454c3e6ec619b41329fd756188432554f36",
          "message": "tokenize table_tokenize: set query domain",
          "timestamp": "2024-02-28T23:20:12+09:00",
          "tree_id": "f9d6c60eb044650a9eacead4b0c3f2dd1881f726",
          "url": "https://github.com/groonga/groonga/commit/d0fdd454c3e6ec619b41329fd756188432554f36"
        },
        "date": 1709133095279,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36807429900011357,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012508000000000047 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2836052670000413,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016614999999999047 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01609730800004172,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003630000000008071 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015184530000055929,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036200000000086163 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4269024310000304,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022500000000000298 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25827246900018963,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022472000000000242 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15754200199972956,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02297999999999914 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017602560000000267,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019009999999993477 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01673690699999497,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019069999999995202 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08163146199996163,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024852000000000526 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0742667389999383,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024424999999998642 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01765588000000662,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021110000000010842 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026965336999978717,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00198399999999907 s\nthreads: undefined"
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
          "id": "801ed5e8986f607a87212040d96d45abb8f92078",
          "message": "ii: add support for offline index construction with non text based tokenizer\n\nTokenH3Index is one of non text based tokenizeres.",
          "timestamp": "2024-02-28T23:22:39+09:00",
          "tree_id": "0a1cb25171024cc84ae4c73c74ffe58f1dd57fe5",
          "url": "https://github.com/groonga/groonga/commit/801ed5e8986f607a87212040d96d45abb8f92078"
        },
        "date": 1709134246003,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3737721810000494,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01508199999999904 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27530994199992165,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015524999999999817 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015083950999951412,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003969999999995366 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015279376999956185,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003229999999998512 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4232911740000418,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019700000000000273 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2570432839993373,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022578999999998753 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15087311600029807,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022263000000001434 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016934676999994736,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017709999999988568 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01666879799995513,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018729999999997915 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08036762599942904,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02434800000000173 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07588294000004225,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02409900000000241 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018297276999987844,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019849999999999868 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026959883000131413,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018669999999995635 s\nthreads: undefined"
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
          "id": "4cf8e0bf3770670709d339b8f94c19475fa1590f",
          "message": "ii: add support for online index construction with non text based tokenizer\n\nTokenH3Index is one of non text based tokenizeres.",
          "timestamp": "2024-02-28T23:25:52+09:00",
          "tree_id": "1807331c3914b7a4380890f94a46c7d13c49d55d",
          "url": "https://github.com/groonga/groonga/commit/4cf8e0bf3770670709d339b8f94c19475fa1590f"
        },
        "date": 1709135928029,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37053398400001925,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017235000000000195 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26146913800010907,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014024999999998816 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015838698000038676,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003589999999995541 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024982985000008284,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039300000000075386 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5218381309999813,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019300000000002648 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2576593399999183,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02504900000000114 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15322958700011213,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025027000000001798 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017946147999964523,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018409999999991766 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016398068999933457,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017670000000000186 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0875762120001582,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030017000000000432 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07518859800006794,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02479100000000145 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01824512500002129,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021549999999999347 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02723844800010511,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002067000000000263 s\nthreads: undefined"
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
          "id": "535844b78a8066d61ec3dcacc4a16c7b0cd8ae0c",
          "message": "select: add support for searching by index with non text based tokenizer\n\nTokenH3Index is one of non text based tokenizers.\n\nWe need to accept non text query (such as WGS84GeoPoint query but it's\nnot required for TokenH3Index). So this adds\ngrn_search_optarg::query_domain that shows query type.\n\ngrn_obj_search_column_index_by_key() and grn_ii_sel() use it to\nprocess query with suitable type.\n\nSee the added test which shows that we can search by H3 index with\nWGS84GeoPoint query. (The test uses text because we don't have\nWGS84GeoPoint literal notation.)",
          "timestamp": "2024-02-28T23:27:16+09:00",
          "tree_id": "b3d2a35c2a6dfd4dd563268a06a18bb975a1905f",
          "url": "https://github.com/groonga/groonga/commit/535844b78a8066d61ec3dcacc4a16c7b0cd8ae0c"
        },
        "date": 1709136183938,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36731841299996404,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01858299999999985 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26649862600015695,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013836000000000875 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016405824999935703,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038099999999818834 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015253542000039033,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003680000000003958 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6919478390000222,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020599999999998397 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25795209600005364,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025632999999999545 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15549497499995368,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02825900000000145 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017180696000025364,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019709999999999728 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017139337999935833,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021890000000004406 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.10039026599980616,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03596300000000163 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08419162000012648,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02885300000000124 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01817537699997729,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021979999999988398 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0251359960000741,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019060000000001576 s\nthreads: undefined"
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
          "id": "371b1d13fffd112536858ca0a51d673f4d8809b1",
          "message": "packages h3: disable on Debian bullseye/Ubuntu 20.04/CentOS 7\n\nH3 requires CMake 3.20 or later but they don't provide it.",
          "timestamp": "2024-02-28T23:53:12+09:00",
          "tree_id": "a72d45195332e9c77db781c0541790d283eaf594",
          "url": "https://github.com/groonga/groonga/commit/371b1d13fffd112536858ca0a51d673f4d8809b1"
        },
        "date": 1709137274816,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37756945599971914,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021281999999999662 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2597670499999367,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012295999999999543 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015193608000004133,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00033199999999933283 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015249862000018766,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003279999999987737 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.459514845000001,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018799999999999373 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2533256709999705,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0230140000000012 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14935833800024056,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022486999999998453 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017432196000015665,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017379999999995732 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01630421500004786,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001644000000000645 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08026136099999803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02426200000000192 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07503181100003076,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025398999999999672 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017876336999961495,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019439999999999458 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026632375999952274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017339999999985423 s\nthreads: undefined"
          }
        ]
      }
    ]
  }
}