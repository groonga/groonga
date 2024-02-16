window.BENCHMARK_DATA = {
  "lastUpdate": 1708051713348,
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
      }
    ]
  }
}