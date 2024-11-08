window.BENCHMARK_DATA = {
  "lastUpdate": 1731104201352,
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
      },
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
          "id": "b63a88d2af1255a09576069a6ac627b70d9ff889",
          "message": "Revert \"Update encoder.hpp\"\n\nThis reverts commit 625086ee585a22be22f767823033a2b23f3c0679.",
          "timestamp": "2024-10-31T09:40:56-06:00",
          "tree_id": "66a93d942cc1e81d68734b95b0a8dabe39333cba",
          "url": "https://github.com/novatel/novatel_edie/commit/b63a88d2af1255a09576069a6ac627b70d9ff889"
        },
        "date": 1730389389163,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 80781228.25000022,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 80776843.00000001 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 132.44096400710256,
            "unit": "ns/iter",
            "extra": "iterations: 5306102\ncpu: 132.4320962921557 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 141.57813068029057,
            "unit": "ns/iter",
            "extra": "iterations: 4970589\ncpu: 141.57253074032073 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 142.63511685883168,
            "unit": "ns/iter",
            "extra": "iterations: 4902967\ncpu: 142.63201669519697 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 132.40580123875245,
            "unit": "ns/iter",
            "extra": "iterations: 5296455\ncpu: 132.3974212940543 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145599.7709333875,
            "unit": "ns/iter",
            "extra": "iterations: 4789\ncpu: 145592.5786176655 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 407.74672839366696,
            "unit": "ns/iter",
            "extra": "iterations: 1717887\ncpu: 407.72587952525413 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5555.614149081855,
            "unit": "ns/iter",
            "extra": "iterations: 126199\ncpu: 5555.294423886084 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5149.2297591865945,
            "unit": "ns/iter",
            "extra": "iterations: 135333\ncpu: 5149.065955827478 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.9495455273226,
            "unit": "ns/iter",
            "extra": "iterations: 1786026\ncpu: 391.93338618810645 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5482.184996050827,
            "unit": "ns/iter",
            "extra": "iterations: 127873\ncpu: 5482.044254846604 ns\nthreads: 1"
          }
        ]
      },
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
          "id": "5ff82ea3f33a6e47bee38bb092a70b48d53fc899",
          "message": "Reapply \"Update encoder.hpp\"\n\nThis reverts commit b63a88d2af1255a09576069a6ac627b70d9ff889.",
          "timestamp": "2024-10-31T09:46:12-06:00",
          "tree_id": "40c90cf7038781a6fe0f07be857e4ca922d73a91",
          "url": "https://github.com/novatel/novatel_edie/commit/5ff82ea3f33a6e47bee38bb092a70b48d53fc899"
        },
        "date": 1730389704522,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 80221982.57142626,
            "unit": "ns/iter",
            "extra": "iterations: 7\ncpu: 80218914 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 119.82352609170844,
            "unit": "ns/iter",
            "extra": "iterations: 5775953\ncpu: 119.81899177503699 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 118.43367938788836,
            "unit": "ns/iter",
            "extra": "iterations: 5836074\ncpu: 118.43089909415123 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 122.00339226072717,
            "unit": "ns/iter",
            "extra": "iterations: 5751032\ncpu: 121.99858112422257 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 118.48742102168087,
            "unit": "ns/iter",
            "extra": "iterations: 5577480\ncpu: 118.48072749700589 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145744.40674770132,
            "unit": "ns/iter",
            "extra": "iterations: 4772\ncpu: 145736.91701592624 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 408.4870552317806,
            "unit": "ns/iter",
            "extra": "iterations: 1711116\ncpu: 408.46439341342085 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5488.671020966798,
            "unit": "ns/iter",
            "extra": "iterations: 130063\ncpu: 5488.489147566946 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 4938.246096176422,
            "unit": "ns/iter",
            "extra": "iterations: 141656\ncpu: 4937.876489523916 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.04881704962605,
            "unit": "ns/iter",
            "extra": "iterations: 1787818\ncpu: 391.0265222746391 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5233.830706648153,
            "unit": "ns/iter",
            "extra": "iterations: 132994\ncpu: 5233.647781102918 ns\nthreads: 1"
          }
        ]
      },
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
          "id": "8fe2ee59f0ad763b4b69d2f1b4c58b90bd2185f0",
          "message": "Revert \"Reapply \"Update encoder.hpp\"\"\n\nThis reverts commit 5ff82ea3f33a6e47bee38bb092a70b48d53fc899.",
          "timestamp": "2024-10-31T09:57:57-06:00",
          "tree_id": "66a93d942cc1e81d68734b95b0a8dabe39333cba",
          "url": "https://github.com/novatel/novatel_edie/commit/8fe2ee59f0ad763b4b69d2f1b4c58b90bd2185f0"
        },
        "date": 1730390411438,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 81651696.62499493,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 81646475.375 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 132.3838056018442,
            "unit": "ns/iter",
            "extra": "iterations: 5335524\ncpu: 132.37808901243815 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 133.08882117243405,
            "unit": "ns/iter",
            "extra": "iterations: 5261482\ncpu: 133.08458130237824 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 134.83685087999083,
            "unit": "ns/iter",
            "extra": "iterations: 5152360\ncpu: 134.83078860949163 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 132.15605742024286,
            "unit": "ns/iter",
            "extra": "iterations: 5310462\ncpu: 132.1523502098311 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146406.19240453147,
            "unit": "ns/iter",
            "extra": "iterations: 4766\ncpu: 146400.56294586655 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 402.6894921740877,
            "unit": "ns/iter",
            "extra": "iterations: 1734945\ncpu: 402.6750484885681 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5665.277141581631,
            "unit": "ns/iter",
            "extra": "iterations: 123215\ncpu: 5665.183825021299 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5332.038030536909,
            "unit": "ns/iter",
            "extra": "iterations: 131447\ncpu: 5331.778146325141 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 389.5769891401342,
            "unit": "ns/iter",
            "extra": "iterations: 1797812\ncpu: 389.56716497609307 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5598.105887342563,
            "unit": "ns/iter",
            "extra": "iterations: 124963\ncpu: 5597.706105007076 ns\nthreads: 1"
          }
        ]
      },
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
          "id": "865d2417e9529ea6a521d2d7952aeddc72409497",
          "message": "Reapply \"Reapply \"Update encoder.hpp\"\"\n\nThis reverts commit 8fe2ee59f0ad763b4b69d2f1b4c58b90bd2185f0.",
          "timestamp": "2024-10-31T10:12:04-06:00",
          "tree_id": "40c90cf7038781a6fe0f07be857e4ca922d73a91",
          "url": "https://github.com/novatel/novatel_edie/commit/865d2417e9529ea6a521d2d7952aeddc72409497"
        },
        "date": 1730391255053,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 81135134.87500156,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 81132400.375 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 114.50057978119754,
            "unit": "ns/iter",
            "extra": "iterations: 6110926\ncpu: 114.49935836238238 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 114.80250780360458,
            "unit": "ns/iter",
            "extra": "iterations: 6102950\ncpu: 114.79906831941926 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 116.39612640756181,
            "unit": "ns/iter",
            "extra": "iterations: 6014830\ncpu: 116.39182886299368 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 115.33968082837092,
            "unit": "ns/iter",
            "extra": "iterations: 6047530\ncpu: 115.33580255079359 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 148161.49530716415,
            "unit": "ns/iter",
            "extra": "iterations: 4688\ncpu: 148158.70477815703 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 408.68145160583293,
            "unit": "ns/iter",
            "extra": "iterations: 1713523\ncpu: 408.67386489705723 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5460.3184901508,
            "unit": "ns/iter",
            "extra": "iterations: 128993\ncpu: 5459.993844627226 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 4978.900794243265,
            "unit": "ns/iter",
            "extra": "iterations: 140637\ncpu: 4978.716603738701 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 392.97580648322236,
            "unit": "ns/iter",
            "extra": "iterations: 1786057\ncpu: 392.9588926893146 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5290.464413757542,
            "unit": "ns/iter",
            "extra": "iterations: 131652\ncpu: 5290.385022635414 ns\nthreads: 1"
          }
        ]
      },
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
          "id": "a39ca20a790b7702a788851a952bbdf1b8e72e94",
          "message": "meaningless change",
          "timestamp": "2024-10-31T10:20:47-06:00",
          "tree_id": "3a5964a34d2a95bf04767e410264127ad72d6508",
          "url": "https://github.com/novatel/novatel_edie/commit/a39ca20a790b7702a788851a952bbdf1b8e72e94"
        },
        "date": 1730391775018,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 79803239.87500171,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 79801476.62499999 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 147.93518502537742,
            "unit": "ns/iter",
            "extra": "iterations: 4693113\ncpu: 147.92963135556295 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 147.64903892769613,
            "unit": "ns/iter",
            "extra": "iterations: 4692623\ncpu: 147.64550316528727 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 150.80152274965258,
            "unit": "ns/iter",
            "extra": "iterations: 4634511\ncpu: 150.79832456973352 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 147.78094155086816,
            "unit": "ns/iter",
            "extra": "iterations: 4713500\ncpu: 147.77025967964354 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146379.0054736867,
            "unit": "ns/iter",
            "extra": "iterations: 4750\ncpu: 146364.07263157904 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 411.4728311599816,
            "unit": "ns/iter",
            "extra": "iterations: 1714096\ncpu: 411.4566039475038 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5460.5801517894315,
            "unit": "ns/iter",
            "extra": "iterations: 129785\ncpu: 5460.375559579309 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 4910.570572471424,
            "unit": "ns/iter",
            "extra": "iterations: 142173\ncpu: 4910.287600317919 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.8819225122062,
            "unit": "ns/iter",
            "extra": "iterations: 1790553\ncpu: 391.8560003529637 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5267.928369099776,
            "unit": "ns/iter",
            "extra": "iterations: 132387\ncpu: 5267.532635379601 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "committer": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "distinct": true,
          "id": "ae1f158af815f167a39cebb5fc531b6632f29ae9",
          "message": "refactor calculateblockcrc32",
          "timestamp": "2024-10-31T16:00:10-06:00",
          "tree_id": "448db57ec9d72facc418a3dd0e32d1b1cc41eab0",
          "url": "https://github.com/novatel/novatel_edie/commit/ae1f158af815f167a39cebb5fc531b6632f29ae9"
        },
        "date": 1730412133941,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 81562932.87500204,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 81557740.5 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 129.69086621533881,
            "unit": "ns/iter",
            "extra": "iterations: 5323226\ncpu: 129.6831387583394 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 130.57105129030185,
            "unit": "ns/iter",
            "extra": "iterations: 5230658\ncpu: 130.56598978560632 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 132.25240750763325,
            "unit": "ns/iter",
            "extra": "iterations: 5338197\ncpu: 132.24410526625374 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 130.54252399938252,
            "unit": "ns/iter",
            "extra": "iterations: 5389098\ncpu: 130.53950531239178 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145036.78826954812,
            "unit": "ns/iter",
            "extra": "iterations: 4808\ncpu: 145028.46401830288 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 404.40837737309016,
            "unit": "ns/iter",
            "extra": "iterations: 1737824\ncpu: 404.3903997182675 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5556.578148601791,
            "unit": "ns/iter",
            "extra": "iterations: 125786\ncpu: 5556.218808134446 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5155.992689345516,
            "unit": "ns/iter",
            "extra": "iterations: 135282\ncpu: 5155.8572019928715 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 395.5109520690421,
            "unit": "ns/iter",
            "extra": "iterations: 1774094\ncpu: 395.5001431716694 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5464.662490666861,
            "unit": "ns/iter",
            "extra": "iterations: 127235\ncpu: 5464.515950799708 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "committer": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "distinct": true,
          "id": "f976a0736ce8f27e78897b63d0f521f93d0605fe",
          "message": "refactor calculateblockcrc32",
          "timestamp": "2024-10-31T16:47:22-06:00",
          "tree_id": "911c38d8ae29baf9f841c44f9b45a0269ccdcb54",
          "url": "https://github.com/novatel/novatel_edie/commit/f976a0736ce8f27e78897b63d0f521f93d0605fe"
        },
        "date": 1730414965829,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 78687232.49999832,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 78684697.37500001 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 123.89468351206499,
            "unit": "ns/iter",
            "extra": "iterations: 5669758\ncpu: 123.88957888502468 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 123.47143005140755,
            "unit": "ns/iter",
            "extra": "iterations: 5743325\ncpu: 123.46369306978096 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 123.95747400113335,
            "unit": "ns/iter",
            "extra": "iterations: 5287730\ncpu: 123.95310880094091 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 124.12119528374116,
            "unit": "ns/iter",
            "extra": "iterations: 5651177\ncpu: 124.11253974172108 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145563.45833333104,
            "unit": "ns/iter",
            "extra": "iterations: 4392\ncpu: 145560.1179417122 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 406.6174705566643,
            "unit": "ns/iter",
            "extra": "iterations: 1712527\ncpu: 406.5906990079576 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5586.739745527503,
            "unit": "ns/iter",
            "extra": "iterations: 125043\ncpu: 5586.467023343957 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5177.466605781048,
            "unit": "ns/iter",
            "extra": "iterations: 134679\ncpu: 5177.250180057771 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.6554180520606,
            "unit": "ns/iter",
            "extra": "iterations: 1794621\ncpu: 391.629711231508 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5455.86333556276,
            "unit": "ns/iter",
            "extra": "iterations: 127085\ncpu: 5455.753173073142 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "jonathan.mcdermid1@ucalgary.ca",
            "name": "Jonathan McDermid"
          },
          "committer": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "distinct": true,
          "id": "9f5abcdbcd57e417a311bbebf7b9b5c6aa44c819",
          "message": "refactor encoder",
          "timestamp": "2024-11-08T14:54:15-07:00",
          "tree_id": "03efeb33c4a098236ae1d651fcee2bd4e981d153",
          "url": "https://github.com/novatel/novatel_edie/commit/9f5abcdbcd57e417a311bbebf7b9b5c6aa44c819"
        },
        "date": 1731102977010,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 74999639.75000413,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74997089.625 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 129.7722424693638,
            "unit": "ns/iter",
            "extra": "iterations: 5444663\ncpu: 129.7664624973116 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 129.71217340924352,
            "unit": "ns/iter",
            "extra": "iterations: 5286777\ncpu: 129.70400680792855 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 132.14140710056603,
            "unit": "ns/iter",
            "extra": "iterations: 5295215\ncpu: 132.13343934098998 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 129.43561888405733,
            "unit": "ns/iter",
            "extra": "iterations: 5331144\ncpu: 129.43394475932374 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 153660.75665557015,
            "unit": "ns/iter",
            "extra": "iterations: 4808\ncpu: 153657.85004159732 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 408.08981218109477,
            "unit": "ns/iter",
            "extra": "iterations: 1718464\ncpu: 408.08538264403575 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5633.951251106277,
            "unit": "ns/iter",
            "extra": "iterations: 124290\ncpu: 5633.8130259876 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5258.028658628573,
            "unit": "ns/iter",
            "extra": "iterations: 132700\ncpu: 5257.960738507909 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 395.8556217732427,
            "unit": "ns/iter",
            "extra": "iterations: 1792237\ncpu: 395.8487175524219 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5641.633677856825,
            "unit": "ns/iter",
            "extra": "iterations: 120940\ncpu: 5641.472118405813 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "committer": {
            "email": "jonathan.mcdermid@hexagon.com",
            "name": "Jonathan McDermid",
            "username": "jonathanmcdermid"
          },
          "distinct": true,
          "id": "9936bd6e7552bf329ebea0a8301f0483e75c002b",
          "message": "Update benchmark.yml",
          "timestamp": "2024-11-08T15:14:36-07:00",
          "tree_id": "33e0c24ca4f1ac28cb4fca97c9b1a2d17b507b16",
          "url": "https://github.com/novatel/novatel_edie/commit/9936bd6e7552bf329ebea0a8301f0483e75c002b"
        },
        "date": 1731104199850,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "LoadJson",
            "value": 75855312.87499946,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75850030.12500001 ns\nthreads: 1"
          },
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 122.73699344886253,
            "unit": "ns/iter",
            "extra": "iterations: 5714428\ncpu: 122.73494967475308 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 122.78636922252876,
            "unit": "ns/iter",
            "extra": "iterations: 5670667\ncpu: 122.78626588371345 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 123.51462033022203,
            "unit": "ns/iter",
            "extra": "iterations: 5675590\ncpu: 123.51456571034899 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 123.32190929969535,
            "unit": "ns/iter",
            "extra": "iterations: 5580224\ncpu: 123.31872466051536 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 148683.87743674923,
            "unit": "ns/iter",
            "extra": "iterations: 4822\ncpu: 148683.89713811703 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 402.92113465523255,
            "unit": "ns/iter",
            "extra": "iterations: 1730887\ncpu: 402.9154941945945 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5602.167228266055,
            "unit": "ns/iter",
            "extra": "iterations: 124644\ncpu: 5601.93349860402 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5253.13375801091,
            "unit": "ns/iter",
            "extra": "iterations: 129667\ncpu: 5253.122421279122 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 391.12030602318356,
            "unit": "ns/iter",
            "extra": "iterations: 1792936\ncpu: 391.1203093696601 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5777.591484510363,
            "unit": "ns/iter",
            "extra": "iterations: 126311\ncpu: 5771.096404905358 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}