window.BENCHMARK_DATA = {
  "lastUpdate": 1711784839555,
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
          "id": "c17b7ba73ca718a4586b49a60d28844e69adff82",
          "message": "packages rpm: H3 4.1.0 still always uses \"lib\" as library dir\n\nWe can remove \"/usr/lib/\" patterns after we use H3 that includes\nhttps://github.com/uber/h3/commit/094699baf7fbaf6fca4697f6dc4ce78b5bcd1c38\n.",
          "timestamp": "2024-02-29T06:34:03+09:00",
          "tree_id": "e499dd745cfafa16e94954830621b1c8b3338f30",
          "url": "https://github.com/groonga/groonga/commit/c17b7ba73ca718a4586b49a60d28844e69adff82"
        },
        "date": 1709156448927,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37710941399984677,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020190000000001207 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2868448149999381,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016572000000001016 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01570140400008313,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003449999999993736 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015068939999935083,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032599999999938234 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4625343289999932,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023799999999971067 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2546836879997727,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02578999999999998 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15165917100023307,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025768999999999542 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017110529000092356,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018099999999998395 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01637582199998633,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001689999999999442 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08519632899998442,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028551000000000562 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07724718300016775,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02427399999999788 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018628001000024597,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002060000000000256 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01733137299987675,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019909999999992156 s\nthreads: undefined"
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
          "id": "66d614272c0971ca1e579713d1b8110e5b8c4661",
          "message": "cmake h3: disable more needless options",
          "timestamp": "2024-02-29T06:39:31+09:00",
          "tree_id": "87b8f90230722c525be603dd30b4a1c6a0279cf3",
          "url": "https://github.com/groonga/groonga/commit/66d614272c0971ca1e579713d1b8110e5b8c4661"
        },
        "date": 1709156666939,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3791105080000534,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021957000000000657 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26030072799972004,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012683000000000166 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01570066799996539,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004350000000010734 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015579601000013099,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003589999999995541 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3693175810000184,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.000290000000000179 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.255597423000097,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0239289999999987 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15546391999993148,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023557000000000022 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016545755000038298,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016459999999999808 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01649500299998863,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016789999999992644 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08301834800028018,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025137999999999563 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07988139699995145,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025567999999995594 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018334705999961898,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021769999999996237 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02667668099996945,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018780000000013786 s\nthreads: undefined"
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
          "id": "2fe1c1fd4a35299e71f0534eb0cf48fe6f38028d",
          "message": "Use bool instead of grn_bool in expr.{c,h}\n\nNot all of expr.c has been replaced.\nIn this commit, only some of it was replaced.\n\nGH-1638\n\nSigned-off-by: Abe Tomoaki <abe@clear-code.com>",
          "timestamp": "2024-02-29T08:28:53+09:00",
          "tree_id": "868e9751aa41d3b6d3147c90d173863ce8147ce4",
          "url": "https://github.com/groonga/groonga/commit/2fe1c1fd4a35299e71f0534eb0cf48fe6f38028d"
        },
        "date": 1709163516490,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37019572600001993,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018544999999998896 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27576513100018474,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01660100000000203 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01589032099997212,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003859999999997754 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015066676000003554,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00045899999999957086 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6612582149999753,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002529999999994481 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2629673109999544,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029578999999999855 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15706236800025408,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0293560000000003 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017198024999970585,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020060000000006184 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01671817999999803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021020000000007144 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09798849099991003,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03710799999999956 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08520833599993693,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030707000000000456 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017568997000012132,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019819999999997895 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027200806999985616,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021079999999997767 s\nthreads: undefined"
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
          "id": "6216438f80a52e1dd4eb210a92bb1cb1bd8519da",
          "message": "grn_obj_search_column_index_by_key: add missing NULL check",
          "timestamp": "2024-02-29T09:16:53+09:00",
          "tree_id": "ded06d58e78706ac2af2d065a98045ebc4ec40e4",
          "url": "https://github.com/groonga/groonga/commit/6216438f80a52e1dd4eb210a92bb1cb1bd8519da"
        },
        "date": 1709166119621,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37850593100017704,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015233999999999318 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2781742509999958,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013780999999999738 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016330725999864626,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004890000000004335 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015179815000124108,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036600000000003297 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4345394960000704,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00025799999999942536 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26431790200001615,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025792999999999205 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15880770199976268,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024526999999999716 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017137613999921086,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018530000000002433 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.0165715180000916,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017680000000002138 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08733322600039628,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02954999999999977 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08117648400036614,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027925999999995454 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01830789600001026,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021209999999994844 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02720892099989669,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017759999999997222 s\nthreads: undefined"
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
          "id": "c641c0787f6c55d878dfd26fb7394a6a76cb39f8",
          "message": "cmake h3: fix variable name to build static library",
          "timestamp": "2024-02-29T09:43:40+09:00",
          "tree_id": "bb634a27a3a51fcfa1707f4446edbe2ce07d5828",
          "url": "https://github.com/groonga/groonga/commit/c641c0787f6c55d878dfd26fb7394a6a76cb39f8"
        },
        "date": 1709168046095,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38150151799993637,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01937699999999963 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2734647939997785,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013853999999998923 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015409670000053666,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039899999999981617 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024924980999912805,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004049999999985454 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4930257199999915,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023400000000051158 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2631976239994174,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02502899999999922 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16436895699985143,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027361000000000885 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01688463999994383,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020570000000006416 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01632969199999934,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016519999999987656 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08576105700024073,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027665000000001744 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07718879100070808,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02544499999999819 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018362750999926902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020090000000000108 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026544582000042283,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017180000000003304 s\nthreads: undefined"
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
          "id": "d99ec3d43a4b457b62d5ccdd71d766319cbe7cc8",
          "message": "geo: export msec/radian conversion macros",
          "timestamp": "2024-02-29T11:48:49+09:00",
          "tree_id": "f0ffdccd3eef6c5b70f666aa225f4f15df6911a3",
          "url": "https://github.com/groonga/groonga/commit/d99ec3d43a4b457b62d5ccdd71d766319cbe7cc8"
        },
        "date": 1709175310169,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3822696449998375,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015112000000000486 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27828711199981626,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015076000000000589 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016206808000049477,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00033599999999989194 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015265416000033838,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036300000000011323 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4638424819999614,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002019999999998412 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26204815900001677,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022960999999999288 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15915531999985433,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02542900000000131 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017419108000012784,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020210000000009387 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016651805000094555,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00187200000000054 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08997942899992495,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03168200000000021 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07887215600021591,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025273999999996022 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01827321100000745,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019669999999999965 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017351742000016657,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019970000000002486 s\nthreads: undefined"
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
          "id": "17862e1130ce81378c86289f3b2ffb3d720fa30f",
          "message": "clang-format: add lib/grn_geo.h",
          "timestamp": "2024-02-29T11:49:47+09:00",
          "tree_id": "f7cce0a1816e96179a8dd4c20c08ee5fe8b2cc25",
          "url": "https://github.com/groonga/groonga/commit/17862e1130ce81378c86289f3b2ffb3d720fa30f"
        },
        "date": 1709175391983,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.369929337999622,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016926000000000968 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.272424357000034,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013390000000000984 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015426740999942012,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003790000000005733 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015241918000072019,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000355000000000133 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.437284209999973,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002119999999993516 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2643534889999728,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025384999999999935 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15606954399976303,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022032000000001217 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018431763999956274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0032700000000001617 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016444315000001097,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016720000000003121 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0821397219997948,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02469499999999876 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07390264999975216,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023457000000000117 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.009198076000018318,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018660000000003951 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017895882999937385,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017720000000006064 s\nthreads: undefined"
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
          "id": "62b3205964dddef34819d0143949e5adb53ec150",
          "message": "clang-format: add lib/geo.c",
          "timestamp": "2024-02-29T11:50:51+09:00",
          "tree_id": "dc80a2a1a65c47ca5fb04b104465b8cd31ef5428",
          "url": "https://github.com/groonga/groonga/commit/62b3205964dddef34819d0143949e5adb53ec150"
        },
        "date": 1709176517570,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.39065824500011104,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02239499999999818 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2813052689998585,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01642800000000061 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016070420999994894,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000381000000000159 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015368791000014426,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004609999999996006 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4337574080000195,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019800000000000373 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26517741000054684,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025950999999999252 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16185287500007917,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028107999999998273 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01711162399993782,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001956999999999681 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016953301999990344,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001929000000000014 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09538972599978024,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03500199999999817 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08056002999967404,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02728500000000414 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018005307000009907,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019720000000016946 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01769393199992919,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002174999999999372 s\nthreads: undefined"
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
          "id": "3fcf8c81a3d1e6812741f8912cd1d5a6386a1eca",
          "message": "geo: export GRN_GEO_RESOLUTION\n\nIt's used by GRN_GEO_RADIAN2MSEC()/GRN_GEO_MSEC2RADIAN().\n\nmath.h is needed for M_PI.",
          "timestamp": "2024-02-29T11:53:28+09:00",
          "tree_id": "e3ea12efc20b02a036f230ad56316beb1b44f533",
          "url": "https://github.com/groonga/groonga/commit/3fcf8c81a3d1e6812741f8912cd1d5a6386a1eca"
        },
        "date": 1709177674188,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.39072443900033704,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021551999999999488 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2889705040000763,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01893200000000206 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01595592800003942,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004399999999991633 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01530687600006786,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004110000000006053 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.653005324999981,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022399999999997422 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2641431330002888,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026295000000000054 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15972163699984776,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025396000000000196 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017251392000048327,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019680000000003306 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016487241000049835,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018730000000011515 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0956090479995737,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03472200000000057 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08173171400017054,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028493999999995884 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017904436999856443,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020109999999991246 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02700459800007593,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00185600000000119 s\nthreads: undefined"
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
          "id": "760df417318db14c88dd487b29fcb7905bff61e9",
          "message": "h3: use GRN_GEO_MSEC2RADIAN()",
          "timestamp": "2024-02-29T11:54:19+09:00",
          "tree_id": "5d0c474a59354af8b46a1bd1f529dc33424b6ac6",
          "url": "https://github.com/groonga/groonga/commit/760df417318db14c88dd487b29fcb7905bff61e9"
        },
        "date": 1709178480255,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37478581599998506,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017746999999999763 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2832969460001209,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01772299999999921 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016012467999985347,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000438000000001576 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.021713939999983722,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034699999999976416 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3244724540000448,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0001860000000001305 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26230802900010985,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02279599999999933 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15890596799994228,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024951999999999586 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01750133400003051,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018860000000006927 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016713519000006727,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018420000000011483 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08249688400007926,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024541000000001145 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07293317500017338,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02264099999999919 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01756772499999215,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019380000000004116 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026840109999966444,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001848000000000738 s\nthreads: undefined"
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
          "id": "de46a80580ac05c6e0e39a5a3893f66ca4066b47",
          "message": "h3: include math.h explicitly for M_PI\n\ngroonga.h includes math.h. So we don't need this in h3_index.c but\nVisual C++ build didn't work with it...",
          "timestamp": "2024-02-29T13:13:09+09:00",
          "tree_id": "5766cad6364387b086f5306c4e3bc72d8c0056c8",
          "url": "https://github.com/groonga/groonga/commit/de46a80580ac05c6e0e39a5a3893f66ca4066b47"
        },
        "date": 1709184954336,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37163994799976763,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015240000000000684 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.272406098999852,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013551999999999759 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015672113000050558,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034000000000045105 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015281674000050316,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034600000000040154 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3796981879999635,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00015899999999999248 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26553419599997596,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025701999999998892 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15915254599985929,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024743999999999627 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01730548499995166,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020449999999989643 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016454545000044618,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018769999999993792 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08381670600005009,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028010999999998523 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07701697899966575,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02427099999999957 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018394839000109187,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019479999999996167 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0271272359999557,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001982000000000067 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "fb554a68d397a0333c7d9fa6d73ac188c2a63fa0",
          "message": "doc news: add 14.0.0 entry (#1709)",
          "timestamp": "2024-02-29T17:56:30+09:00",
          "tree_id": "719664c1caf8d6b0e6504195ca3e2f534e626871",
          "url": "https://github.com/groonga/groonga/commit/fb554a68d397a0333c7d9fa6d73ac188c2a63fa0"
        },
        "date": 1709197574651,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3820200740000246,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01919200000000003 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2722491120000541,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012994999999997203 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015376087000049665,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034599999999934683 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015194853000025432,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003330000000003608 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3831481469999858,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020999999999979369 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2607464400000481,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02291200000000035 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15721306000023105,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022990000000001343 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017532243999994535,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018839999999997192 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016505394000034812,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016969999999991714 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0824640260001388,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025305000000002326 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07603836000032516,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024173000000001152 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01866893699997263,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022790000000007804 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017446462999998857,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019680000000006914 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "af228759c7b140b072789591a7dd8ae4c42b64c4",
          "message": "h3: add missing definition for using common math constants in VC++ (#1719)\n\nWe want use `M_PI` macro in math.h.\r\nBut we can't use `M_PI` in VC++ just because we include `math.h`. We\r\nneed to define `_USE_MATH_DEFINES` before including `math.h` in order to\r\nuse `M_PI`.\r\n\r\nSee:\r\nhttps://learn.microsoft.com/en-us/cpp/c-runtime-library/math-constants\r\n\r\n`math.h` is included in `groonga.h`.\r\nSo, we define `_USE_MATH_DEFINES` before including `groonga.h`",
          "timestamp": "2024-02-29T18:21:07+09:00",
          "tree_id": "0daf69e5486c146f62f2ee294e34be8651241f21",
          "url": "https://github.com/groonga/groonga/commit/af228759c7b140b072789591a7dd8ae4c42b64c4"
        },
        "date": 1709199353198,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36870717200037006,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01606999999999975 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27594179099997973,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014163999999998705 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01693082399992818,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004889999999990735 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015311410999970576,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032399999999999096 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3384743530000378,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020300000000000873 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26230696900000794,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02351999999999893 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15710072999985414,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023149000000000752 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01734275199999047,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017639999999987666 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016664431999970475,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018559999999999965 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0821383190002507,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024126999999999316 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07593341399922338,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02470299999999906 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017180080000116504,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002074000000000825 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.023937277000072754,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017660000000009335 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "distinct": true,
          "id": "3f5a26faace23d41eac09627565d7be8bd2de580",
          "message": "dist: include XSimd source archive in Groonga source archive\n\nBecause we fail build package for Ubuntu on the Launchpad without\nXSimd source archive.\n\nIf XSimd source archive does not exist under the `groonga/vendor`\ndirectory, Groonga download XSimd source archive from GitHub.\n\nHowever, Launchpad does not allow access to the Internet.\nSo, we need to XSimd source archive under the `groonga/vendor` directory.",
          "timestamp": "2024-02-29T21:41:42+09:00",
          "tree_id": "55b713d2538e407a5233b7327991aaea0ee99e92",
          "url": "https://github.com/groonga/groonga/commit/3f5a26faace23d41eac09627565d7be8bd2de580"
        },
        "date": 1709213559059,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3734092409995924,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01767099999999963 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26908668999976726,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012881999999999977 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015720100000010007,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034099999999970265 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015180353999994622,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003609999999998337 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3795853360000478,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00013699999999999823 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2632151579998663,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025602000000000402 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15771529700009523,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022928999999998173 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017548575000034816,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018880000000000285 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016522727000051418,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018569999999993314 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0826343739998947,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026060999999998835 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07348909700021977,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023250000000000187 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017404805999944983,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00196400000000016 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02701231199995391,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001971000000000306 s\nthreads: undefined"
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
          "id": "ca8e337c022ba36ca959ec4e63baba8e94395b8b",
          "message": "clang-format: add include/groonga/ii.h",
          "timestamp": "2024-03-04T23:00:45+09:00",
          "tree_id": "d7d0f220befcec7f6e0dee54bf322328e0a3fd4a",
          "url": "https://github.com/groonga/groonga/commit/ca8e337c022ba36ca959ec4e63baba8e94395b8b"
        },
        "date": 1709561213931,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3702205470002582,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016615999999999714 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27083401100026094,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014034000000001642 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01693371700002899,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000433999999999754 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015097529999934522,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003440000000008714 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.403819862999967,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.000209000000000098 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26449832899982084,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026833999999999525 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16629140499992445,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02506599999999906 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017768512000088776,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002077999999999136 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016463641999848733,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018579999999999985 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08864926699993703,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029788999999999205 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0805707539998366,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026733999999998703 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01836943799992241,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001977000000001422 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017613124000149583,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001934000000000019 s\nthreads: undefined"
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
          "id": "6cb19bed0fc80287e009be1a811507f530f68704",
          "message": "ii: add grn_ii_select_by_id()\n\nThis is a public API of grn_ii_at().",
          "timestamp": "2024-03-04T23:01:24+09:00",
          "tree_id": "13c0a3453e29038cc536a3aa369d54410b844867",
          "url": "https://github.com/groonga/groonga/commit/6cb19bed0fc80287e009be1a811507f530f68704"
        },
        "date": 1709562760850,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3793684199999916,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020675000000000332 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28510463600019875,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017499000000001708 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016330739000068206,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003289999999986637 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.0152870779999148,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004380000000008266 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.410384826999973,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019700000000000273 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25467489800013254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02379899999999968 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15606606499966347,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026590999999999587 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01707537200002207,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017030000000008982 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016405798000107552,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016909999999984993 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0871779380002522,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029129000000000765 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07997870600001988,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026419000000000276 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017605355999990024,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019820000000008164 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02673195800002759,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001696000000000808 s\nthreads: undefined"
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
          "id": "4d0518dfddf2fe5eae137ab887d4a52fb02290dc",
          "message": "ii: add grn_ii_get_lexicon()\n\nInternal unused grn_ii_lexicon() is removed.",
          "timestamp": "2024-03-04T23:10:29+09:00",
          "tree_id": "57a2f3a10af14fc69c0a2a7e49ddbcd62f0de917",
          "url": "https://github.com/groonga/groonga/commit/4d0518dfddf2fe5eae137ab887d4a52fb02290dc"
        },
        "date": 1709563777021,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36716190200013443,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020780000000000673 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2758301929999334,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012685000000000224 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01541693099989061,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003660000000000885 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015128235000076984,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003289999999999682 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4289739719999943,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0001939999999996389 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2399694680001403,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024379999999999458 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1497828509997703,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02442400000000032 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017395973000077447,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017270000000007002 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.0167831930000375,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001954999999999374 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08275057300016897,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02646900000000084 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08213157299979912,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028449999999998532 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01842689900007599,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001996999999999971 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01750340099999903,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018070000000000308 s\nthreads: undefined"
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
          "id": "2506243110decb5753adf926a7fc7200b14151f2",
          "message": "Don't use M_PI in math.h\n\nIt's not portable. We define GRN_GEO_M_API instead.",
          "timestamp": "2024-03-04T23:34:19+09:00",
          "tree_id": "00ee9f39d4bc38f4b8edfac69a60eb156d0186a9",
          "url": "https://github.com/groonga/groonga/commit/2506243110decb5753adf926a7fc7200b14151f2"
        },
        "date": 1709565204217,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36878613000033056,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021328000000000916 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2602723370001172,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012291000000000607 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016005731999996442,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003490000000008209 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015413142999989304,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003360000000007801 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4534277409999845,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00015099999999923508 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24158857799960742,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02589500000000028 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14915242999990141,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02329099999999873 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.0167128570000159,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019420000000014426 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.025691772000016044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001816999999998986 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08377051199977359,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027595000000001008 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07828157000022884,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024192000000000685 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017462440999906903,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001989999999999964 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017651485999977012,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018889999999986973 s\nthreads: undefined"
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
          "id": "a89dcb1eafcc87d4af9aebf4a8ad128c71fc82c8",
          "message": "cmake: fix a typo",
          "timestamp": "2024-03-04T23:47:37+09:00",
          "tree_id": "b49dd013e5e868deb66f722a388dd2c9fa8d3ade",
          "url": "https://github.com/groonga/groonga/commit/a89dcb1eafcc87d4af9aebf4a8ad128c71fc82c8"
        },
        "date": 1709565839661,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3503158209999242,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0155700000000015 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28253065499984586,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018193999999998656 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015669309999907455,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003960000000011732 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.023024286000065786,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004430000000004708 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4005201389999797,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019499999999961215 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2377163870001482,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02394400000000052 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15431995299979917,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0256010000000009 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017326613999955498,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018960000000001753 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016553116000011414,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001900999999999431 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08332070900007693,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026252999999999874 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0795017410000014,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026025000000002074 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018238906999954452,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002246999999999999 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02665823900002806,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020559999999997802 s\nthreads: undefined"
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
          "id": "0c00253eeb1adde3fa7e8838f33ea68046cadd99",
          "message": "h3: add grn_h3_geo_point_to_cell()",
          "timestamp": "2024-03-05T00:34:29+09:00",
          "tree_id": "69e38bfe71c5e98b1fe0c699bee9520be3213ea8",
          "url": "https://github.com/groonga/groonga/commit/0c00253eeb1adde3fa7e8838f33ea68046cadd99"
        },
        "date": 1709568381334,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3860782560000189,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02363399999999999 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2777090589997897,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017202999999999552 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016716948999999204,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004180000000003903 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015385729000001902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003680000000007566 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6510252969999897,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002430000000000765 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2629824170001598,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031688000000001354 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16486939899999697,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028988999999999626 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017024279999986902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017790000000000028 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017779955000094105,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001929000000000014 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.11829469400021253,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.042546000000001694 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.10579297799995402,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.040132999999998975 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.019915149999974346,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002886000000000194 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01744261800001823,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017469999999999986 s\nthreads: undefined"
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
          "id": "44e9edda7549efee9a973898a4711a22588520be",
          "message": "expr: need math.h for fpclassify()",
          "timestamp": "2024-03-05T01:08:36+09:00",
          "tree_id": "9fad897f24c8bafaf049d8657d712c642d420e67",
          "url": "https://github.com/groonga/groonga/commit/44e9edda7549efee9a973898a4711a22588520be"
        },
        "date": 1709570616785,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37002171199986833,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016830999999998084 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2747531179997509,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016285999999999357 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016280013000027793,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003470000000004858 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.022842758000081176,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037299999999973465 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3725728060000506,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020500000000001073 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25364442899979167,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02413099999999993 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15011489299985215,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023626000000000827 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017322619000026407,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018039999999992506 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016448270999944725,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017339999999999578 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08364749799983429,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026601000000000735 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07582163300020284,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02582299999999746 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018212672999936785,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021000000000002406 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026919559000020854,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002005999999999647 s\nthreads: undefined"
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
          "id": "a2c5714a4589744ccbbb27ad0779961872eb6666",
          "message": "Use bool instead of grn_bool in io.{c,h}\n\nThe change in db.c is where it relates to `grn_io_is_corrupt`.\n\nGH-1638",
          "timestamp": "2024-03-05T08:09:36+09:00",
          "tree_id": "e3ea70c72aa43870735b45a153924845c95af6be",
          "url": "https://github.com/groonga/groonga/commit/a2c5714a4589744ccbbb27ad0779961872eb6666"
        },
        "date": 1709594661576,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37311670300010746,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017690999999999735 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27609052100001463,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01600700000000134 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016053690999967785,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003939999999991173 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015448471999945923,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004900000000009896 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3622582039999998,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021000000000004349 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2585231819998626,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024575999999999917 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14716454700004533,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022234999999999644 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016477410999982567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002004999999999396 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01667958900003441,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018100000000005334 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08044528099969739,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025062000000001222 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07945494400001962,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02661600000000175 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01794684599997254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021730000000004523 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017634093999959077,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018819999999993842 s\nthreads: undefined"
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
          "id": "c127c9f9a554ee3f1f508f3ca9afee5bc26b7fa7",
          "message": "h3: add missing \"#ifdef GRN_WITH_H3\"",
          "timestamp": "2024-03-05T09:30:18+09:00",
          "tree_id": "1197a5aef5d903dc15e217a654adc248f4288073",
          "url": "https://github.com/groonga/groonga/commit/c127c9f9a554ee3f1f508f3ca9afee5bc26b7fa7"
        },
        "date": 1709598968843,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36549781999974584,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015858000000000483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26336233000000675,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014073000000001473 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015515423000124429,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039000000000138924 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015488589999961277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003499999999991843 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3863591470000074,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018400000000001748 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2534467979998567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023975999999999636 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14915888999996696,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023890999999998608 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01761491800004933,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001958999999999378 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016359032999901046,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016579999999999373 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08241482699986591,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02632300000000032 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07491107199967928,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0240089999999982 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01756478099986225,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020669999999997635 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017523893000031876,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001773000000001912 s\nthreads: undefined"
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
          "id": "76dd2e9ca2a8ed41a1087474fe2a17462be185a8",
          "message": "geo: re-introduce fallback MIN/MAX definitions\n\nThey are needed on Windows.",
          "timestamp": "2024-03-05T09:36:08+09:00",
          "tree_id": "28ce259902ab377bd1440c5e67722a4de0fe5b09",
          "url": "https://github.com/groonga/groonga/commit/76dd2e9ca2a8ed41a1087474fe2a17462be185a8"
        },
        "date": 1709599335384,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37085219399989455,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01429599999999942 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2781383600001277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016970000000000277 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015148632000091311,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004140000000001365 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015027569000039875,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003640000000000032 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5641897919999792,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023300000000014975 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2567819570001575,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027055999999998498 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1521474230001445,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02613800000000116 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016738668999948914,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018620000000004744 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.019117161999986365,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.004380999999998497 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08804067899973234,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029510000000000314 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07915686200016125,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025752000000001357 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01801390199995012,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021209999999998175 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017441348000090784,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018019999999996095 s\nthreads: undefined"
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
          "id": "8f40efb8f49e013f70ed9fbe6b14b11a65134ca7",
          "message": "h3: don't undeclared variable",
          "timestamp": "2024-03-05T13:31:31+09:00",
          "tree_id": "a2609c48280ad4c15d742984c825a475db871325",
          "url": "https://github.com/groonga/groonga/commit/8f40efb8f49e013f70ed9fbe6b14b11a65134ca7"
        },
        "date": 1709613396917,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3823388630002569,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022446999999999273 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2621619679997593,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011996999999997926 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015674446000105036,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035100000000110043 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015086119999921266,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036100000000072185 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.368117878000021,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0001949999999996399 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2526076070000727,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023031000000001106 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14777277700000013,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022269000000000982 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01664267199993219,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017279999999981754 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01620142600006602,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001613000000000031 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0801222079998638,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023776999999999063 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07888953200000515,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025013000000000604 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01769395100006932,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002075999999999939 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026835113000004185,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017029999999982337 s\nthreads: undefined"
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
          "id": "b2b1010d38cdfe8ff4f442432a8b1ef40e153219",
          "message": "Use bool instead of grn_bool in grn_table_is_grouped and grn_table_tokenize\n\nGH-1638",
          "timestamp": "2024-03-06T08:28:23+09:00",
          "tree_id": "492485cc38b9e0903fbfb968f759641814f6ac22",
          "url": "https://github.com/groonga/groonga/commit/b2b1010d38cdfe8ff4f442432a8b1ef40e153219"
        },
        "date": 1709681974656,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3666930899997851,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016769999999999688 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26351248000014493,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012456000000002632 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016212152999969476,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003390000000003113 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015145910000001095,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032499999999924256 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.333663193999996,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018399999999998973 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2512057540001251,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022468000000000557 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1480963260006547,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02259699999999959 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017368442000019968,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002002999999999866 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01652232800006459,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018269999999986908 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.07931513000050927,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02565200000000048 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07741261099999974,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025241999999999515 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017750550000187104,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021549999999990466 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017433995999908802,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017639999999998213 s\nthreads: undefined"
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
          "id": "129aae9dc7c6def1be87ab78d4238797f1565416",
          "message": "Use bool instead of grn_bool in alloc.{c,h}\n\nGH-1638",
          "timestamp": "2024-03-07T08:13:09+09:00",
          "tree_id": "abe30e473c939152bdb089f7e1ac6f86a0b45fdb",
          "url": "https://github.com/groonga/groonga/commit/129aae9dc7c6def1be87ab78d4238797f1565416"
        },
        "date": 1709767412312,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3775532139999882,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01974299999999922 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2790042049998078,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01796399999999823 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015065501999913522,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037800000000043354 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01525526899996521,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034699999999965314 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4036220529999923,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019700000000000273 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2551020540001332,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024250000000000563 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1494227939995767,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02375000000000005 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.005569152000020949,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017750000000005817 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016709371000047213,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017209999999998615 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0818906340000467,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026514000000000662 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07798016299989285,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026339999999999947 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01814397500004361,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021210000000000118 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026739158999930623,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001834999999999809 s\nthreads: undefined"
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
          "id": "ac2c7036231401ada80200ebf59b404b9f0cdb3a",
          "message": "between: add missing NULL check\n\nIf a request is canceled between grn_expr_create() and\ngrn_expr_add_var() in GRN_EXPR_CREATE_FOR_QUERY(), expr isn't NULL but\nvariable is NULL. So we need to check not only expr but also variable.",
          "timestamp": "2024-03-07T17:48:47+09:00",
          "tree_id": "d51827f310d47efda0053bd70cb5cb49fe520f97",
          "url": "https://github.com/groonga/groonga/commit/ac2c7036231401ada80200ebf59b404b9f0cdb3a"
        },
        "date": 1709802035089,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3669973870000831,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017633000000000315 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2899897469998223,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016368999999999856 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01571798700013005,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003449999999993736 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015144013000053747,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032500000000013074 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3286436189999904,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021000000000001573 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2519669519998615,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023017000000001064 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14658136399975774,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022752999999999496 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016828570000029686,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001718999999999582 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01646860999983346,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019179999999998643 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0783955130002596,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024247999999998812 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07386602500014305,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02326100000000013 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017427513999905386,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001990000000001796 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01736716899995372,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002017999999999992 s\nthreads: undefined"
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
          "id": "5401e746af2e955f501ace72cbd8d255ed4bc51f",
          "message": "h3 grn_h3_compute_cell: validate resolution",
          "timestamp": "2024-03-07T23:12:36+09:00",
          "tree_id": "cecc037155029747d7b077c64c35b481dfb692e0",
          "url": "https://github.com/groonga/groonga/commit/5401e746af2e955f501ace72cbd8d255ed4bc51f"
        },
        "date": 1709821125281,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3612096869998709,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015828000000000952 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2673067109999465,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01590200000000236 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01596365200003902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003580000000003025 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024969446999989486,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003719999999995949 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4889943119999884,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019400000000030504 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24384860900011063,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026572999999999722 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14885433899991085,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026273000000000823 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017619715999956043,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002083000000000057 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016928780999933224,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020020000000001426 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08615401500009057,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02699899999999965 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0745839719999708,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02393399999999893 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017782259000000522,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021359999999992496 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02728482099996654,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019809999999999828 s\nthreads: undefined"
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
          "id": "0466adea77552b5808cddde5789e395865fa5f06",
          "message": "h3: add grn_h3_compute_grid_disk()\n\nIt's a wrapper for gridDisk().",
          "timestamp": "2024-03-07T23:12:09+09:00",
          "tree_id": "e47954cef7c84a53b6767385fe66b6bbc90288a4",
          "url": "https://github.com/groonga/groonga/commit/0466adea77552b5808cddde5789e395865fa5f06"
        },
        "date": 1709821137696,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3721021119998227,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02136600000000108 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2638588100003858,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013157999999998421 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01610570499997266,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003369999999994766 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015782011000055718,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003630000000010014 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6091085950000092,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020999999999998797 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2481614799999079,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026825000000000015 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1476779449999981,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025883000000000253 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017959880999910638,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019140000000000268 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01682434899993268,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019410000000007477 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0994345050002039,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03547200000000052 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08251387600000726,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027640999999999055 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01756869100000813,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002000999999999503 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.020307591000005232,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002151999999999571 s\nthreads: undefined"
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
          "id": "cb9c81b6e79258afb68b96ad1825fd79f1669e6f",
          "message": "functions/h3: fix style",
          "timestamp": "2024-03-08T00:14:13+09:00",
          "tree_id": "cacece4d1287239b09fea6668b704fd54a3f46d1",
          "url": "https://github.com/groonga/groonga/commit/cb9c81b6e79258afb68b96ad1825fd79f1669e6f"
        },
        "date": 1709824776748,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3805641230001129,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021489000000000508 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27266501199994764,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01741300000000065 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015640562000044156,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00044699999999914253 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024923381000007794,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034000000000133923 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4102355039999566,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021300000000001873 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25770105699893975,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02649499999999959 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1474479630010137,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024380000000002483 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016914836999603722,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017529999999990054 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01654714299979787,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018059999999999743 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08526124099989829,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026824999999998766 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07781949699960933,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025763000000003505 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01759449999985918,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002043999999999782 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027220016000001124,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016710000000000197 s\nthreads: undefined"
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
          "id": "56312f7611e4af275286ccef8a35f616730e396b",
          "message": "h3_grid_distance: add\n\nIt computes H3's grid distance:\nhttps://h3geo.org/docs/api/traversal#griddistance",
          "timestamp": "2024-03-08T00:38:56+09:00",
          "tree_id": "c7a3ab4fe6a8322bf0be0289a329f90fb5efdbb2",
          "url": "https://github.com/groonga/groonga/commit/56312f7611e4af275286ccef8a35f616730e396b"
        },
        "date": 1709827017644,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3682531689994448,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018855999999999262 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26892469299991717,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015256000000000824 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015956139000081748,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040200000000065184 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.020018016999983956,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00045400000000020424 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5566646559999526,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0001880000000005766 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25426553300053456,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02728900000000009 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1515158909993488,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025118000000001722 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016877075000252262,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002007999999999982 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01658111500000814,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019400000000002748 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09516137399964464,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0340770000000008 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0845143030003328,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029441999999996804 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018432781999990766,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002150999999999098 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017381063000016184,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002026000000001832 s\nthreads: undefined"
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
          "id": "e8c313e5bf3078c636053f475cb3536540516bf5",
          "message": "Use bool instead of grn_bool in pat.c\n\nGH-1638",
          "timestamp": "2024-03-08T07:55:37+09:00",
          "tree_id": "33e5604ba623fc4464cd16f67db3d5bcfd2d0b5e",
          "url": "https://github.com/groonga/groonga/commit/e8c313e5bf3078c636053f475cb3536540516bf5"
        },
        "date": 1709853090167,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3777369119997047,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022336000000000633 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.277834319999954,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017350999999999478 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015954351000061706,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00048099999999884346 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024472724000020207,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00039500000000014523 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.395045281000023,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023899999999998922 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25156608500003586,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024101000000000566 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14837494300007847,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022207999999999867 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016456262000019706,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018039999999992506 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016384574000085195,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00172299999999867 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08090365199973348,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025373000000001186 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07531872399982831,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023318000000001393 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018470000000093023,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002054000000000389 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01719230700001617,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017649999999997945 s\nthreads: undefined"
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
          "id": "eebbb4d8a419df8beefb40bfb74559ff6db49be5",
          "message": "normalizers: fix a bug that checks are invalid\n\nIf this is happen, highlight may return invalid result because\nhighlight uses checks.\n\nIt's happen when multiple normalizers such as NormalizerTable and\nNormalizerNFKC150 are used and REMOVE_BLANK flags is\nused (grn_pat_scan(), which is used for highlight, uses it).\n\ngrn_string_open_() merges checks by each normalizer. The merge logic\nis buggy. If 'NormalizerTable(...), NormalizerNFKC150' is used as\nnormalizers, checks by NormalizerNFKC150 is overwritten by checks by\nNormalizerTable unconditionally. It may produces invalid checks.\n\nThe new merge logic is choosing larger check but I'm not sure this is\ncorrect merge logic... We need more cases to consider the correct\nmerge logic.",
          "timestamp": "2024-03-08T16:53:18+09:00",
          "tree_id": "20b30e28aa8d4aacfa9e24be18e7d9813aa7a456",
          "url": "https://github.com/groonga/groonga/commit/eebbb4d8a419df8beefb40bfb74559ff6db49be5"
        },
        "date": 1709885035387,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36480123000001186,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017735999999999502 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27306557599968073,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016878000000002225 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015174224000020331,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003769999999998497 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015251691999878858,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037199999999878997 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.372358403000021,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019399999999999973 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25207517200010443,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023393000000000358 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15034101500043562,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023736999999999453 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01801939900008165,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001870999999999512 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016582139000092866,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017930000000010438 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08345617700024377,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026806999999999664 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0780471910001097,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024763000000001895 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018081217000030847,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019360000000002708 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.007429748000049585,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017930000000015434 s\nthreads: undefined"
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
          "id": "114ddf77c8bfebb4d8f2d10c0274e3e1686845c4",
          "message": "Use bool instead of grn_bool in accessor.c\n\nGH-1638",
          "timestamp": "2024-03-10T09:24:25+09:00",
          "tree_id": "6bdd0d29a73dd251b9ae40f05df110638a2deaaf",
          "url": "https://github.com/groonga/groonga/commit/114ddf77c8bfebb4d8f2d10c0274e3e1686845c4"
        },
        "date": 1710030756226,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3583938200001171,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015600999999999865 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2613623399999483,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.012443999999999067 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.007273855999983425,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00028500000000025727 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01497203899998567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00031900000000018025 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3096863569999755,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002110000000001 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25146487999990086,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025572999999999485 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15122817599996097,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024769000000000096 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01735017499999003,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020799999999988605 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016596907000007377,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019589999999984897 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0836066689999484,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026698999999998807 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08067138700005216,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027483999999999148 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018011025000021164,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019780000000002573 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017283798999926603,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019269999999999843 s\nthreads: undefined"
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
          "id": "7ad663550f9af2b841fc16a39ff93103b31c1f7c",
          "message": "Use bool instead of grn_bool in proc_tokenize.c\n\nGH-1638",
          "timestamp": "2024-03-11T09:19:01+09:00",
          "tree_id": "753157fa8143f317776052294c02fdad953220f8",
          "url": "https://github.com/groonga/groonga/commit/7ad663550f9af2b841fc16a39ff93103b31c1f7c"
        },
        "date": 1710116810759,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.35716011800002434,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014883000000000743 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27475390699999025,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016421000000000602 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015549955999972553,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032500000000013074 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.014977239000131704,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003229999999998512 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4005149270000175,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018899999999999473 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24962656899947433,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02336900000000007 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14928696500021488,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023115000000002273 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01612066500013043,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018109999999998405 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016296850999992785,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001747999999998695 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.07787973199992848,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024792000000000078 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07822651200001474,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024959000000000398 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017950718000179222,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019919999999999938 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02658870499999466,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018059999999999743 s\nthreads: undefined"
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
          "id": "6e029b479394c51de9b73c424f5844ec42c601de",
          "message": "Use bool instead of grn_bool in grn_select_sort\n\nRemoved unnecessary `else`.\n\nGH-1638",
          "timestamp": "2024-03-12T08:25:13+09:00",
          "tree_id": "3c6fbc02944fdb4a4f91436f9dd500bc367d22ab",
          "url": "https://github.com/groonga/groonga/commit/6e029b479394c51de9b73c424f5844ec42c601de"
        },
        "date": 1710199994102,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36054080200005956,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016025000000000483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27342800600024475,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015628000000000475 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015660816000092836,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00033400000000050056 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024606009000024187,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004019999999993473 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5470086369999763,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019499999999977868 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2532211199998642,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025137000000001006 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15078468600023598,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02419799999999958 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016459838999935528,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017480000000005547 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016820305000010194,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018880000000000008 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09369420900003433,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03243900000000041 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08428674199990382,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028884000000002852 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01744609600007152,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018830000000000235 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026778548999914165,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018590000000009432 s\nthreads: undefined"
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
          "id": "7ef02d58d008de2367e8d8d710e11af017d5235d",
          "message": "test: use too long key for invalid key\n\nInstead of a key that becomes an empty key after normalization.\n\nWe'll change the behavior of the case to a valid case.",
          "timestamp": "2024-03-12T17:22:44+09:00",
          "tree_id": "656efb7df8fa832778407f4f788f3c0a652ccb0c",
          "url": "https://github.com/groonga/groonga/commit/7ef02d58d008de2367e8d8d710e11af017d5235d"
        },
        "date": 1710232218046,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3722559229996705,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02002799999999988 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26688831499996013,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013891999999998988 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015537024999957794,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034299999999909403 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015196836999848529,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00031899999999929207 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3944442600000002,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020999999999982144 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25957968900081596,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025404999999999123 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15369220999969002,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025945000000001633 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01666873899989696,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019780000000020337 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01665062500012482,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019290000000011798 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08957394999993085,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030463999999999547 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08199198699992394,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02770900000000112 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018121160999953645,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021650000000000003 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017281070999842996,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018299999999991101 s\nthreads: undefined"
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
          "id": "33ddd76e830856a68ea72cabcea6ca90288d943b",
          "message": "load: Stop reporting an error for key that becomes an empty key by normalization\n\n\"-\" becomes \"\" with NormalizerNFKC150(\"remove_symbol\", true). So the\nfollowing case reports a \"empty key\" error:\n\n    table_create Values TABLE_HASH_KEY ShortText \\\n      --normalizers 'NormalizerNFKC150(\"remove_symbol\", true)'\n    table_create Data TABLE_NO_KEY\n    column_create Data value COLUMN_SCALAR Values\n    load --table Data\n    [\n    {\"value\": \"-\"}\n    ]\n\n\"-\" is treated as \"\" with this change. {\"value\": \"\"} is just ignored.\nSo {\"value\": \"-\"} is also just ignored.",
          "timestamp": "2024-03-12T17:25:31+09:00",
          "tree_id": "4fc4b8463b92390429cc1456edc7b444e189509d",
          "url": "https://github.com/groonga/groonga/commit/33ddd76e830856a68ea72cabcea6ca90288d943b"
        },
        "date": 1710232544939,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36125156399964453,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015324000000000199 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2620917620002956,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013259999999999744 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016147540000019944,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000344000000000122 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01514040200004274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037400000000117894 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4740179190000617,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021100000000023877 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2541956070002698,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025588999999999723 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15159926199999063,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02441299999999974 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017069345999857433,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016730000000002576 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016292704999955276,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016570000000003526 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08788397000068926,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030204999999998983 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07893653500048003,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02698500000000273 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018488715999751548,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021509999999999863 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026728719999823625,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018209999999988236 s\nthreads: undefined"
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
          "id": "8e1bc798fa18e9accf1dfeea9431191d88606ce0",
          "message": "test: remove needless scalar_ prefix",
          "timestamp": "2024-03-12T17:31:24+09:00",
          "tree_id": "583d74910cd751baeee5721cc0093702f3c93a07",
          "url": "https://github.com/groonga/groonga/commit/8e1bc798fa18e9accf1dfeea9431191d88606ce0"
        },
        "date": 1710233131258,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36238909999974567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01501800000000078 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2665700630006995,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013784000000000268 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015700954999829264,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00045900000000065333 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015041191999898729,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00034399999999834563 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.304064153000013,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018899999999999473 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25107357199954095,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023498000000000283 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15324967300023218,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025607000000000157 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01676836900003309,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016549999999993237 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016548057000136396,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00187599999999985 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0807918390001987,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025260999999999326 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07413567800028886,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023321999999999316 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017462026000202968,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018840000000004409 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02062081200006105,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019609999999996575 s\nthreads: undefined"
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
          "id": "d0fe6b6b7176f48c6844ebdb3911bd4405e88367",
          "message": "test: move to existing directory",
          "timestamp": "2024-03-12T17:34:52+09:00",
          "tree_id": "1b6fd5b3ce7ce7cf9d0ea7195360fdcacddbc907",
          "url": "https://github.com/groonga/groonga/commit/d0fe6b6b7176f48c6844ebdb3911bd4405e88367"
        },
        "date": 1710235301907,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36055461700016167,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015331999999999874 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27214195299984567,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014648999999998857 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01542296900004203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035799999999966414 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015170152000109738,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032999999999994145 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3889599980000185,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019000000000010675 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25616319300002033,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02704599999999953 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15036699500024042,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024030000000000995 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01688423700011299,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018880000000005837 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01641406500039011,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018539999999998003 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08216505199902713,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026880999999999405 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07707476999894425,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024848999999998345 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017101946000366297,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019489999999993124 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02678760499998134,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001965000000000605 s\nthreads: undefined"
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
          "id": "a5ab856b833303517fab0db6cb63ad13cbc65eaf",
          "message": "load: ensure initializing grn_table_add_options by 0",
          "timestamp": "2024-03-13T05:18:45+09:00",
          "tree_id": "bd29682881e9c3c927a0fb787c40ddfec355e34c",
          "url": "https://github.com/groonga/groonga/commit/a5ab856b833303517fab0db6cb63ad13cbc65eaf"
        },
        "date": 1710275032547,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36581229399973836,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016892000000000684 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28153828299969064,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018403000000001807 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015570725999964452,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038100000000079737 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01520824800002174,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003790000000005733 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3661190170000737,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018999999999999573 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26987522900049044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02282899999999935 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1491786179999508,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023562000000001443 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017036200999996254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017889999999995965 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01631492999990769,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016419999999977009 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08111769199979335,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026561000000000015 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07856272099991202,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02527399999999788 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.016909779999878083,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019530000000003156 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026654828000005182,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018069999999999475 s\nthreads: undefined"
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
          "id": "82d9413a53480955a47be85b919d26717c954b22",
          "message": "test load: add a test for empty key by normalization with \"[...]\"\n\nIt should be ignored by 33ddd76e830856a68ea72cabcea6ca90288d943b .",
          "timestamp": "2024-03-13T09:38:26+09:00",
          "tree_id": "7c9b1935a205efcce5c741c34fb823362b747425",
          "url": "https://github.com/groonga/groonga/commit/82d9413a53480955a47be85b919d26717c954b22"
        },
        "date": 1710295008319,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36305664400003934,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016278000000000403 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26221053899996605,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013011999999997442 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015424931999973523,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036899999999917554 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01529239499996038,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037200000000048306 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3967408240000054,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002100000000008484 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25141512899978125,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02395899999999984 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15193067299992435,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025057000000000967 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01700515199996744,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019470000000003096 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016530743999965125,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018009999999979154 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09532455899983461,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03463800000000064 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08190245299999788,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027109000000000716 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017527915999949073,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021209999999998175 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.02500447800002803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018620000000010295 s\nthreads: undefined"
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
          "id": "b7b4711b0cd8160f2610dfa67cdf0ade8e7b4374",
          "message": "test load: add a test for empty key by normalization with [...]\n\nIt should be ignored by 33ddd76e830856a68ea72cabcea6ca90288d943b .",
          "timestamp": "2024-03-13T09:35:46+09:00",
          "tree_id": "9548d06142e38c1044a6a76d640d922c4b7f2a50",
          "url": "https://github.com/groonga/groonga/commit/b7b4711b0cd8160f2610dfa67cdf0ade8e7b4374"
        },
        "date": 1710295014485,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36728163099991207,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018606000000000122 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28215619200005904,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019092000000001164 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01520254000007526,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035499999999996645 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015146133999962785,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035799999999941434 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.519564516999992,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021799999999999597 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.27207055499991384,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029306999999999223 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1637583870002004,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03078400000000081 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017119554999965203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019519999999993987 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.022285736000071665,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002203000000001981 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0997556769999619,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.035159000000001564 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0852004139999849,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02879299999999993 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017826516000013726,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001980000000000648 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.023218521999979203,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.005881999999999804 s\nthreads: undefined"
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
          "id": "dd8c5a4950802106dfbb3d32007f13618233f048",
          "message": "text-bulk: add support for specifying flags",
          "timestamp": "2024-03-13T10:11:07+09:00",
          "tree_id": "acaa811946be122bb83cc5af82eedebd0271c78a",
          "url": "https://github.com/groonga/groonga/commit/dd8c5a4950802106dfbb3d32007f13618233f048"
        },
        "date": 1710295779298,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36932928399846787,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017634000000000483 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.261860516003253,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01167999999999976 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015316047000396793,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035600000000091114 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015198516999589629,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003390000000008109 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3589018110001234,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020200000000000773 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24994343700063837,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022358999999998852 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.148279473999537,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.021754000000001245 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016850670999701833,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016670000000003071 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016658878999805893,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018829999999997737 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.07645490299864832,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023939000000000862 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07644978999860541,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022983999999999394 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01727634799954103,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001976999999999396 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.023767971999404836,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017000000000003679 s\nthreads: undefined"
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
          "id": "6cabf0a42e46fd51012733653d265f21ded57d7b",
          "message": "test load vector reference object: add empty w/wo normalized key case\n\nHmm. This doesn't report \"failed to cast\" error. Let's changing\njson-object behavior to align to this behavior.",
          "timestamp": "2024-03-13T10:26:51+09:00",
          "tree_id": "db0fe0049ecdbbad294579b0c5c95ea07e2389fa",
          "url": "https://github.com/groonga/groonga/commit/6cabf0a42e46fd51012733653d265f21ded57d7b"
        },
        "date": 1710296897083,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3698018659998752,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01660899999999911 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27052767100042274,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013515000000000013 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015583735999825876,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003449999999993736 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015377534999970521,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00044200000000049755 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.395570512000063,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00024100000000001898 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25755124900024384,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022693000000000352 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15347680599973046,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023312000000000277 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.018357218999994984,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0031350000000004152 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01651213200022994,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017659999999999343 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08543752299942753,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02531400000000003 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07569148099958056,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024322999999997763 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017969322999988435,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002177999999999236 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026567290000002686,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018310000000001936 s\nthreads: undefined"
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
          "id": "851dab0b9a70566605a8c74b323c462def048226",
          "message": "Fix style",
          "timestamp": "2024-03-13T11:03:57+09:00",
          "tree_id": "625a11e384d2c88695767f4a7b92ff11316483ce",
          "url": "https://github.com/groonga/groonga/commit/851dab0b9a70566605a8c74b323c462def048226"
        },
        "date": 1710299405542,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.376517258000149,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019320999999999006 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2794589159999532,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016721000000000652 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015966641000147774,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038899999999930657 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024585756999954356,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005159999999992948 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4623974109999835,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022199999999972242 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25530915599995296,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02305499999999927 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.153039922999767,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02308000000000046 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01712391299997762,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020249999999984447 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01630159100000128,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016890000000003846 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.07950804599988714,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024738999999998304 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07956808500023271,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025296000000001012 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017224335000037172,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001927000000000234 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026688400999944406,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019809999999991224 s\nthreads: undefined"
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
          "id": "3b8e9b27c59e2316c2a04aeef6ede2552ad6a23a",
          "message": "load vector reference json-object: accept empty w/wo normalized key\n\nThis is the same behavior as object case.",
          "timestamp": "2024-03-13T11:06:17+09:00",
          "tree_id": "cfc25a345adbc4293182619f86dfa28a7890ed44",
          "url": "https://github.com/groonga/groonga/commit/3b8e9b27c59e2316c2a04aeef6ede2552ad6a23a"
        },
        "date": 1710300035369,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36863515899995036,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01570600000000008 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2693493319999334,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013436000000002085 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.014923693000014282,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00035899999999866594 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024345276999952148,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003319999999999157 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4214186160000395,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019300000000002648 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2577878900000883,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02316499999999941 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15277789500004246,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02343200000000037 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017035293999981604,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019419999999999438 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016389898999989327,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018490000000002393 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08185668700031101,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024840999999998947 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0747543269998232,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023663000000000753 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017415863000053378,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018930000000002556 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026435665000065,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017899999999994032 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b16070a81f1ab77a7e6728ea418276f83e9d994f",
          "message": "test: skip a test that uses an empty key in Apache Arrow input mode (#1737)\n\nBecause Apache Arrow format can't send an empty key.",
          "timestamp": "2024-03-14T05:38:10+09:00",
          "tree_id": "1b40abba7ec25f78baa2ab7197c75fb8b8a6378d",
          "url": "https://github.com/groonga/groonga/commit/b16070a81f1ab77a7e6728ea418276f83e9d994f"
        },
        "date": 1710362670201,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37579070599963416,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016677000000000927 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28546932099970945,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017542000000001445 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01646525900002871,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037300000000062283 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.016667188000042188,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032999999999994145 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4673580689999426,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020699999999954088 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2658813430000464,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024459999999999732 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16766665900001954,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024795999999999485 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01830271200003608,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020149999999998502 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.020413742999949136,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001887999999999973 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09685721600010311,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027464999999999046 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08026045400004023,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024651000000001394 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01939871000013227,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002098000000000766 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.030353215999866734,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017469999999991104 s\nthreads: undefined"
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
          "id": "367a0ecbd9f38d0b10a4b847e8b764ce63bd75e0",
          "message": "Use bool instead of grn_bool in file_reader.c",
          "timestamp": "2024-03-14T06:57:38+09:00",
          "tree_id": "d210002a179bcc318fa657054cba715d3c8301c8",
          "url": "https://github.com/groonga/groonga/commit/367a0ecbd9f38d0b10a4b847e8b764ce63bd75e0"
        },
        "date": 1710367539976,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3869675049999728,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022379999999999928 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2883572409999715,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019185000000000646 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01625945299997511,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0004719999999988622 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01535410799993997,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003680000000005623 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.6876459000000068,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00022900000000009024 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.26016163999986475,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02801700000000057 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15679391199995507,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027126000000002842 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017935252000029323,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00210799999999961 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01679157699999223,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019670000000003296 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09816704999997228,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03638199999999915 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08529123899978686,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03028300000000156 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017873587000053703,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021489999999999843 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017455717999951048,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019710000000001393 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "distinct": true,
          "id": "e5219485ebf0389382a9e8c7880d960eb0f890fb",
          "message": "doc news: add a new news item",
          "timestamp": "2024-03-15T11:15:00+09:00",
          "tree_id": "b4eb87e7aec27eb4daa84a97d49057d3477ae123",
          "url": "https://github.com/groonga/groonga/commit/e5219485ebf0389382a9e8c7880d960eb0f890fb"
        },
        "date": 1710470100974,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.38380558000000065,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02200099999999952 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27423025799993184,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015207999999998834 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015771012999948653,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003829999999993561 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.024940923999963616,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00041799999999891924 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.541060754,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00024200000000054733 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25760519599992904,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025851999999999792 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15666594499975872,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02739500000000128 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01698152099993422,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017520000000011415 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016458521000060955,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017960000000006304 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09378106299999445,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03380599999999842 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08064934400027823,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027443999999995777 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017733668000062153,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021040000000005776 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.027046930000039993,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018729999999997915 s\nthreads: undefined"
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
          "id": "ee3b674e2858c2cae1e7f961ab91cffde5e60de3",
          "message": "Use bool instead of grn_bool under src/suggest",
          "timestamp": "2024-03-19T17:46:56+09:00",
          "tree_id": "c6b24942b524cd86aeea1d5260c487313a1a3108",
          "url": "https://github.com/groonga/groonga/commit/ee3b674e2858c2cae1e7f961ab91cffde5e60de3"
        },
        "date": 1710838795860,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36361323899984654,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014425000000000465 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.26996224500004473,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014496999999999247 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015384068000003026,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000364000000000253 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015129146999981913,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00031600000000242545 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3903956750000361,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00018999999999999573 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2570021460001044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024466000000001834 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15146071100008385,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022034000000000026 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017246544000045105,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018560000000002186 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016342226999995546,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001913000000000581 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09002530700013267,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.030101000000000572 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07717994699976316,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02553100000000122 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017487848000030226,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002005999999999425 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026516099999980725,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018499999999992411 s\nthreads: undefined"
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
          "id": "0e1ea10131d3c0dbf55aa9ac3428efc15c371bdc",
          "message": "Use bool instead of grn_bool in config.{c,h}\n\nGH-1638",
          "timestamp": "2024-03-22T07:13:19+09:00",
          "tree_id": "adc3a7ea4981fd745d6ea25adb8ca12b131d4eff",
          "url": "https://github.com/groonga/groonga/commit/0e1ea10131d3c0dbf55aa9ac3428efc15c371bdc"
        },
        "date": 1711059704752,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37853504600047927,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.016933000000000156 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.28574663199913175,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01735500000000037 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015520470999945246,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036700000000042254 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015174846000036268,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00044300000000063733 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3962867569999844,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002229999999996679 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25835782200010726,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025621999999999187 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15237948400056212,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023090000000000638 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017473440000003393,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018099999999998673 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016338072999928954,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017609999999997628 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08583894100024736,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025853999999999308 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07886866700016526,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025397000000000225 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017609356000093612,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001988000000000767 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026623423999808438,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001762999999999959 s\nthreads: undefined"
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
          "id": "78b97b2dccd4c00d2627c5ce1d5ee4f08db60396",
          "message": "db: reduce log level for setting normalizers/tokenizer/token_filters against temporary table\n\nThey are useless information. So NOTICE isn't suitable.\n\nPGroonga sets normalizers on start. So these logs are noisy.",
          "timestamp": "2024-03-22T15:08:19+09:00",
          "tree_id": "90ace7df75ba1ebc9c5cf7e62427707e37b307f0",
          "url": "https://github.com/groonga/groonga/commit/78b97b2dccd4c00d2627c5ce1d5ee4f08db60396"
        },
        "date": 1711088063994,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3588361560002227,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014449999999999491 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2642965130001471,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.011932999999998445 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015983270999925026,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003759999999997099 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.025357982999992146,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00038800000000005497 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3730810919999499,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002059999999994011 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.2504668740002103,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02743899999999863 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.15169869900023514,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02773100000000009 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017856422000022576,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020340000000009795 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.017039414999942437,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002078000000000302 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0933032920002006,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03413499999999868 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08466445399983513,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029453000000001284 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018629344000032688,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002094999999999611 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017822871000078067,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020469999999994937 s\nthreads: undefined"
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
          "id": "cf514d53ac331f200e7c02481646299916482e65",
          "message": "test: update expected",
          "timestamp": "2024-03-22T21:39:23+09:00",
          "tree_id": "d2d1c3934c42eb30b179a9f0b0407a848a157e13",
          "url": "https://github.com/groonga/groonga/commit/cf514d53ac331f200e7c02481646299916482e65"
        },
        "date": 1711111499344,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.37031168999953934,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020434000000000715 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2741833680008767,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018648000000000914 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015535628000066026,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003549999999991893 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015470503000074132,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040899999999952086 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3799341999999797,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020700000000042906 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24812178499951187,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023695000000000535 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14388805299984142,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.022048000000000734 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016350342000009732,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001690999999999998 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016855005000252277,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018789999999999918 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0842883099996925,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024736000000000535 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07731745100022636,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02481000000000108 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018416418999890993,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020239999999989156 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0172066199999108,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0016570000000003526 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "otegami@clear-code.com",
            "name": "takuya kodama",
            "username": "otegami"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "776c8bc988e52be188c9fec2c141dc977647ae12",
          "message": "doc contribution/documentation: convert introduction from rst to md (#1742)\n\nSince many people are more familiar with Markdown format than\r\nreStructuredText format, I've converted the documentation to Markdown to\r\nmake it easier to maintain.",
          "timestamp": "2024-03-26T15:00:10+09:00",
          "tree_id": "c6c64e510eacf7eef90cf21ba30332ff61366dd8",
          "url": "https://github.com/groonga/groonga/commit/776c8bc988e52be188c9fec2c141dc977647ae12"
        },
        "date": 1711433181845,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.36937072500040813,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02085800000000082 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27719952300049044,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.019453000000001247 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015551686000094378,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003370000000011697 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015567003000001023,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036699999999928457 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4465724879999016,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019899999999997697 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25311101799957214,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02905000000000041 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1474691809999058,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02459499999999845 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01683611200019186,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021060000000012735 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.02139750499998172,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020379999999998732 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.0921501749996878,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03138100000000048 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.08136023800057046,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027888999999998915 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01799889199980953,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018409999999999815 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.026993222000101014,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020690000000005426 s\nthreads: undefined"
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
          "id": "6289549fde3fa633475710ff139f9f2a494d88e2",
          "message": "Use bool instead of grn_bool in window_function_executor.cpp\n\nGH-1638",
          "timestamp": "2024-03-27T08:11:39+09:00",
          "tree_id": "2b6259ff70ce70b969eb4fc25ad668b5dc2cb3e9",
          "url": "https://github.com/groonga/groonga/commit/6289549fde3fa633475710ff139f9f2a494d88e2"
        },
        "date": 1711495321879,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.368945901000302,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.020282999999999995 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2636357779994114,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01536299999999935 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015510105000203112,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00032500000000013074 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015309852999962459,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003919999999988377 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.3151781039999833,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023199999999998222 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24830863100009992,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02516299999999988 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14533401800031243,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02341899999999897 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017331565000063165,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019399999999990813 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016683125999975346,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019119999999990256 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08558615199990527,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02792299999999863 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07740879599998607,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02532800000000343 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017678295000109756,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019400000000004414 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01747655000008308,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0017669999999994357 s\nthreads: undefined"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "committer": {
            "email": "horimoto@clear-code.com",
            "name": "Horimoto Yasuhiro",
            "username": "komainu8"
          },
          "distinct": true,
          "id": "86b9890ec80a2254c7deaa15af29fa2a6889ec69",
          "message": "doc package: update version info to 14.0.2 (2024-03-29)",
          "timestamp": "2024-03-29T11:31:16+09:00",
          "tree_id": "19f8792ef92da12e49d03b59c553ef9665acac46",
          "url": "https://github.com/groonga/groonga/commit/86b9890ec80a2254c7deaa15af29fa2a6889ec69"
        },
        "date": 1711687260697,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.35653294200005803,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.013030000000001096 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2721820890001254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014824999999999228 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01620500600017749,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003859999999997754 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.01678671900003792,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037799999999954537 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.5610253590000411,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00023500000000001298 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25375771999972585,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029458000000000054 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.16451483100024689,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.029034000000000032 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016712628999925982,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001932999999999574 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.018759144999989985,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002069000000000043 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09911063100037154,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.034563000000001134 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0848068660006902,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.03132199999999727 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018952501999820015,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.002098000000000211 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.018721884999990834,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018159999999998178 s\nthreads: undefined"
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
          "id": "063fd25a5af358e4106662e7c359c75d9ac84f2c",
          "message": "db: show ID of dangling reference",
          "timestamp": "2024-03-29T14:15:16+09:00",
          "tree_id": "75209ea660e6e55dbca29cd848de942dd29a8d4a",
          "url": "https://github.com/groonga/groonga/commit/063fd25a5af358e4106662e7c359c75d9ac84f2c"
        },
        "date": 1711689766458,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.35276514800000314,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.015380000000000754 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2736462239998332,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01766499999999957 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016415068000071642,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00040599999999990644 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015369500000019798,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036200000000086163 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4169685059999892,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.0002040000000001485 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24680084700003135,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023126999999999953 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14637605900003337,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02411899999999989 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01739681300000484,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020130000000010695 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.01699964300001966,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001973000000000752 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08550437899987173,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026347999999997845 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07787351000001763,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.025667999999999663 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01747526599996263,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020519999999990546 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.017486017999999603,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001822999999999908 s\nthreads: undefined"
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
          "id": "a4ed2b0f8989c1754a816dbf1b5c79881a951ede",
          "message": "test: update expected",
          "timestamp": "2024-03-29T14:15:58+09:00",
          "tree_id": "638159a03a316f0084d766311729045e676e37f7",
          "url": "https://github.com/groonga/groonga/commit/a4ed2b0f8989c1754a816dbf1b5c79881a951ede"
        },
        "date": 1711690608778,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3545978339999465,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01637400000000011 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.2609268559997986,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014140000000002123 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.016258041000071444,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00037399999999987443 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015583516000049258,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.000398999999998928 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4432192410000084,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00021700000000005049 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24664648199990324,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024853000000000472 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1474049890000515,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.024959000000000064 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.016981794999992417,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019250000000007872 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.020968915000025845,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001839999999999925 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.08477807800005621,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0272060000000008 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.07634143900031631,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.023943999999998522 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.018893799000011313,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022520000000001983 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01752355699989039,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0019930000000004666 s\nthreads: undefined"
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
          "id": "4b798c35346d06e6c3b8f6c4b52867a59df9ee59",
          "message": "test: update expected",
          "timestamp": "2024-03-29T14:17:57+09:00",
          "tree_id": "8f81877aef0731ca0a5869dd960382a29ca3bed2",
          "url": "https://github.com/groonga/groonga/commit/4b798c35346d06e6c3b8f6c4b52867a59df9ee59"
        },
        "date": 1711691053212,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3608133479997946,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.018813999999998998 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27027432599993517,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.01694799999999888 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.015806978000114214,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003910000000004743 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.015305039999987002,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.00036099999999997245 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.4387849390000156,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00019599999999991846 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.24914544999990085,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.027506000000000308 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.1483451920000789,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026317999999998093 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.01659634099996765,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018749999999999878 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016777748000038173,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018510000000011573 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.090103000000056,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.031212000000000295 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0814426800000092,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02885699999999833 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.01800915399991254,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0022059999999999858 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.01746201600008135,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0018020000000005532 s\nthreads: undefined"
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
          "id": "9488876189c103796986e325cb921fff709fd7a4",
          "message": "Change grn_obj_is_table to return bool instead of grn_bool\n\nGH-1638\n\n`lexicon_domain_is_table` was changed to bool since the return value\nof `grn_obj_is_table` is assigned.",
          "timestamp": "2024-03-30T16:34:05+09:00",
          "tree_id": "c4e80e70b73494717d1f64b479fd95ac4fec6c11",
          "url": "https://github.com/groonga/groonga/commit/9488876189c103796986e325cb921fff709fd7a4"
        },
        "date": 1711784837577,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "stdio: json|json: load/data/multiple",
            "value": 0.3587625879997631,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.014535999999999966 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: load/data/short_text",
            "value": 0.27772168799958763,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.017921000000000353 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/multiple",
            "value": 0.01593907199981004,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0005500000000004945 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: select/olap/n_workers/multiple",
            "value": 0.025204655999914394,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0003730000000000122 s\nthreads: undefined"
          },
          {
            "name": "stdio: json|json: wal_recover/db/auto_recovery/column/index",
            "value": 1.605748741999946,
            "unit": "s/iter",
            "extra": "iterations: 1\ncpu: 0.00020299999999998097 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/multiple",
            "value": 0.25173213599964583,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.02833499999999893 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: load/data/short_text",
            "value": 0.14993012999991606,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.026491000000000375 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/multiple",
            "value": 0.017687995000187584,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001887000000000999 s\nthreads: undefined"
          },
          {
            "name": "http: json|json: select/olap/n_workers/multiple",
            "value": 0.016784866999842052,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0020290000000001696 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/multiple",
            "value": 0.09705003900023712,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.035673000000000926 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: load/data/short_text",
            "value": 0.0821869230004495,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.028467999999997634 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/multiple",
            "value": 0.017683681000107754,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.0021590000000000498 s\nthreads: undefined"
          },
          {
            "name": "http: apache-arrow|apache-arrow: select/olap/n_workers/multiple",
            "value": 0.0271376200001896,
            "unit": "s/iter",
            "extra": "iterations: 5\ncpu: 0.001942000000000249 s\nthreads: undefined"
          }
        ]
      }
    ]
  }
}