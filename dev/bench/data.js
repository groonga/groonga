window.BENCHMARK_DATA = {
  "lastUpdate": 1708260789504,
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
      }
    ]
  }
}