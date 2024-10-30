window.BENCHMARK_DATA = {
  "lastUpdate": 1730318374784,
  "repoUrl": "https://github.com/novatel/novatel_edie",
  "entries": {
    "Benchmark": [
      {
        "commit": {
          "author": {
            "email": "jonathan.mcdermid1@ucalgary.ca",
            "name": "Jonathan McDermid"
          },
          "committer": {
            "email": "jonathan.mcdermid1@ucalgary.ca",
            "name": "Jonathan McDermid"
          },
          "distinct": true,
          "id": "2e1cf6ddaff1c53679a4bfa8e609a44304aaae6c",
          "message": "Update build.yml",
          "timestamp": "2024-10-30T13:55:26-06:00",
          "tree_id": "abb1a8e93c84a7ca7e92cb21e375a5f8ecab0d20",
          "url": "https://github.com/novatel/novatel_edie/commit/2e1cf6ddaff1c53679a4bfa8e609a44304aaae6c"
        },
        "date": 1730318252697,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 119.78840082879802,
            "unit": "ns/iter",
            "extra": "iterations: 5674233\ncpu: 119.77976671032012 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 119.04642391540997,
            "unit": "ns/iter",
            "extra": "iterations: 5723774\ncpu: 119.04366961378976 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 118.18936621250596,
            "unit": "ns/iter",
            "extra": "iterations: 5919208\ncpu: 118.17969752034392 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 118.94422446919899,
            "unit": "ns/iter",
            "extra": "iterations: 5647100\ncpu: 118.93965858582287 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 147181.23912134007,
            "unit": "ns/iter",
            "extra": "iterations: 4780\ncpu: 147178.33054393306 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 393.16888836399016,
            "unit": "ns/iter",
            "extra": "iterations: 1778145\ncpu: 393.14255249150153 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5747.87813717351,
            "unit": "ns/iter",
            "extra": "iterations: 125272\ncpu: 5747.632503672006 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5253.053064686542,
            "unit": "ns/iter",
            "extra": "iterations: 134402\ncpu: 5252.706514784005 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 375.1940830165721,
            "unit": "ns/iter",
            "extra": "iterations: 1865748\ncpu: 375.18126081335794 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5464.919656592456,
            "unit": "ns/iter",
            "extra": "iterations: 128710\ncpu: 5464.590482479994 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "novatel",
            "username": "novatel"
          },
          "committer": {
            "name": "novatel",
            "username": "novatel"
          },
          "id": "2e1cf6ddaff1c53679a4bfa8e609a44304aaae6c",
          "message": "benchmarks",
          "timestamp": "2024-10-18T17:41:22Z",
          "url": "https://github.com/novatel/novatel_edie/pull/80/commits/2e1cf6ddaff1c53679a4bfa8e609a44304aaae6c"
        },
        "date": 1730318373238,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 132.44957087509454,
            "unit": "ns/iter",
            "extra": "iterations: 5486165\ncpu: 132.44503200322995 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 127.07102253914344,
            "unit": "ns/iter",
            "extra": "iterations: 5466490\ncpu: 127.06769956590058 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 126.22327173234798,
            "unit": "ns/iter",
            "extra": "iterations: 5556243\ncpu: 126.22116905254144 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 127.50777664946149,
            "unit": "ns/iter",
            "extra": "iterations: 5469322\ncpu: 127.50162085903881 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 148777.06479526707,
            "unit": "ns/iter",
            "extra": "iterations: 4738\ncpu: 148771.8788518361 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 393.01091144936714,
            "unit": "ns/iter",
            "extra": "iterations: 1783906\ncpu: 392.99217223329026 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5625.52535116947,
            "unit": "ns/iter",
            "extra": "iterations: 125438\ncpu: 5625.285152824494 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5220.730071586321,
            "unit": "ns/iter",
            "extra": "iterations: 134243\ncpu: 5220.584946701125 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.7589182207833,
            "unit": "ns/iter",
            "extra": "iterations: 1867553\ncpu: 374.74304879165453 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5434.627474725542,
            "unit": "ns/iter",
            "extra": "iterations: 127994\ncpu: 5434.446966264039 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}