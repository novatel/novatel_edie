window.BENCHMARK_DATA = {
  "lastUpdate": 1730389023031,
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
          "id": "625086ee585a22be22f767823033a2b23f3c0679",
          "message": "Update encoder.hpp",
          "timestamp": "2024-10-31T09:34:54-06:00",
          "tree_id": "40c90cf7038781a6fe0f07be857e4ca922d73a91",
          "url": "https://github.com/novatel/novatel_edie/commit/625086ee585a22be22f767823033a2b23f3c0679"
        },
        "date": 1730389021470,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 80722649.5000002,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 80717914.12500001 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 116.28708944311124,
            "unit": "ns/iter",
            "extra": "iterations: 6096445\ncpu: 116.283481930863 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 116.90605760892977,
            "unit": "ns/iter",
            "extra": "iterations: 6021914\ncpu: 116.90101768308216 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 118.4643070739945,
            "unit": "ns/iter",
            "extra": "iterations: 5898956\ncpu: 118.46226077970402 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 115.1057184712964,
            "unit": "ns/iter",
            "extra": "iterations: 6017255\ncpu: 115.10096131874086 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145613.35837180167,
            "unit": "ns/iter",
            "extra": "iterations: 4766\ncpu: 145606.5297943768 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 409.90884754649164,
            "unit": "ns/iter",
            "extra": "iterations: 1713119\ncpu: 409.89774849266206 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5360.2139213805785,
            "unit": "ns/iter",
            "extra": "iterations: 130375\ncpu: 5360.089794822617 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 4928.00723063255,
            "unit": "ns/iter",
            "extra": "iterations: 141758\ncpu: 4927.938782996376 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.31412956338875,
            "unit": "ns/iter",
            "extra": "iterations: 1788746\ncpu: 391.288829157409 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5292.966793397953,
            "unit": "ns/iter",
            "extra": "iterations: 132564\ncpu: 5292.8274342958875 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}