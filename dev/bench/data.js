window.BENCHMARK_DATA = {
  "lastUpdate": 1707132686234,
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
      }
    ]
  }
}
