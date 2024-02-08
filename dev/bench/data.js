window.BENCHMARK_DATA = {
  "lastUpdate": 1707355189066,
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
      }
    ]
  }
}