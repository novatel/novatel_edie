window.BENCHMARK_DATA = {
  "lastUpdate": 1730342651143,
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
          "id": "78b12f36d9a3f949f285cdaa789e5cfb82201ff5",
          "message": "delete main",
          "timestamp": "2024-10-30T14:09:49-06:00",
          "tree_id": "96d61a3c67d0e04f3c4a5f2e2bf22aff08dc15e5",
          "url": "https://github.com/novatel/novatel_edie/commit/78b12f36d9a3f949f285cdaa789e5cfb82201ff5"
        },
        "date": 1730319113326,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 115.37296718573224,
            "unit": "ns/iter",
            "extra": "iterations: 6012293\ncpu: 115.3654426023482 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 115.80455106669662,
            "unit": "ns/iter",
            "extra": "iterations: 5600665\ncpu: 115.79646434843009 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 119.34299717620114,
            "unit": "ns/iter",
            "extra": "iterations: 5940218\ncpu: 119.33629506526532 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 115.19096614735811,
            "unit": "ns/iter",
            "extra": "iterations: 6121530\ncpu: 115.18360213868093 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 147315.9819744307,
            "unit": "ns/iter",
            "extra": "iterations: 4771\ncpu: 147292.5715782855 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 404.0689644809124,
            "unit": "ns/iter",
            "extra": "iterations: 1730253\ncpu: 404.0523825128466 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5535.255932726599,
            "unit": "ns/iter",
            "extra": "iterations: 126291\ncpu: 5535.032116302815 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5176.566975032373,
            "unit": "ns/iter",
            "extra": "iterations: 135013\ncpu: 5176.222674853527 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 387.32458684963876,
            "unit": "ns/iter",
            "extra": "iterations: 1791539\ncpu: 387.30752051727626 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5431.834112603951,
            "unit": "ns/iter",
            "extra": "iterations: 128539\ncpu: 5431.795346159538 ns\nthreads: 1"
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
          "id": "78b12f36d9a3f949f285cdaa789e5cfb82201ff5",
          "message": "benchmarks",
          "timestamp": "2024-10-18T17:41:22Z",
          "url": "https://github.com/novatel/novatel_edie/pull/80/commits/78b12f36d9a3f949f285cdaa789e5cfb82201ff5"
        },
        "date": 1730319115188,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 121.41320467600033,
            "unit": "ns/iter",
            "extra": "iterations: 5757309\ncpu: 121.40964363733126 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 120.42022092747361,
            "unit": "ns/iter",
            "extra": "iterations: 5676614\ncpu: 120.42022321757304 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 123.52940853105584,
            "unit": "ns/iter",
            "extra": "iterations: 5693807\ncpu: 123.52351967672949 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 122.09460084407536,
            "unit": "ns/iter",
            "extra": "iterations: 5872918\ncpu: 122.08919586481548 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146721.0614291657,
            "unit": "ns/iter",
            "extra": "iterations: 4786\ncpu: 146716.65043877976 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 393.51062189900347,
            "unit": "ns/iter",
            "extra": "iterations: 1699649\ncpu: 393.4858973823424 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5581.945448194973,
            "unit": "ns/iter",
            "extra": "iterations: 125972\ncpu: 5581.88014003111 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5149.4358660250855,
            "unit": "ns/iter",
            "extra": "iterations: 136324\ncpu: 5149.018778791698 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 383.4046539437106,
            "unit": "ns/iter",
            "extra": "iterations: 1824732\ncpu: 383.3935218980105 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5444.2257100964935,
            "unit": "ns/iter",
            "extra": "iterations: 128222\ncpu: 5444.0182417993765 ns\nthreads: 1"
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
          "id": "8b101033eff02472e3e56c52807e47edbedf182d",
          "message": "Update CMakeLists.txt",
          "timestamp": "2024-10-30T14:10:15-06:00",
          "tree_id": "960b151a2896f6b06f206f7aa91aab94636992ab",
          "url": "https://github.com/novatel/novatel_edie/commit/8b101033eff02472e3e56c52807e47edbedf182d"
        },
        "date": 1730319361304,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 122.41021318655075,
            "unit": "ns/iter",
            "extra": "iterations: 5946998\ncpu: 122.40686696043953 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 114.68011121706921,
            "unit": "ns/iter",
            "extra": "iterations: 6155530\ncpu: 114.68047479258486 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 119.43231048960915,
            "unit": "ns/iter",
            "extra": "iterations: 6047584\ncpu: 119.4273579333499 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 120.28367074775458,
            "unit": "ns/iter",
            "extra": "iterations: 5704113\ncpu: 120.27845030419293 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146475.74581525326,
            "unit": "ns/iter",
            "extra": "iterations: 4839\ncpu: 146473.82930357513 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 399.8540287655617,
            "unit": "ns/iter",
            "extra": "iterations: 1757449\ncpu: 399.8366228550581 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5664.468181927791,
            "unit": "ns/iter",
            "extra": "iterations: 124379\ncpu: 5663.883983630673 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5128.397128205106,
            "unit": "ns/iter",
            "extra": "iterations: 136500\ncpu: 5128.083208791202 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 387.1382459842547,
            "unit": "ns/iter",
            "extra": "iterations: 1800725\ncpu: 387.10916825167584 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5478.449704654692,
            "unit": "ns/iter",
            "extra": "iterations: 126970\ncpu: 5478.459250216587 ns\nthreads: 1"
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
          "id": "8b101033eff02472e3e56c52807e47edbedf182d",
          "message": "benchmarks",
          "timestamp": "2024-10-18T17:41:22Z",
          "url": "https://github.com/novatel/novatel_edie/pull/80/commits/8b101033eff02472e3e56c52807e47edbedf182d"
        },
        "date": 1730319567285,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 116.67922728462355,
            "unit": "ns/iter",
            "extra": "iterations: 5651447\ncpu: 116.67268506632018 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 124.11141506177631,
            "unit": "ns/iter",
            "extra": "iterations: 6092363\ncpu: 124.10927139436703 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 119.42505879823037,
            "unit": "ns/iter",
            "extra": "iterations: 5943206\ncpu: 119.42132478665549 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 113.8467634157002,
            "unit": "ns/iter",
            "extra": "iterations: 6096736\ncpu: 113.84048579436599 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 148130.15808358425,
            "unit": "ns/iter",
            "extra": "iterations: 4738\ncpu: 148129.673701984 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 393.78966635164545,
            "unit": "ns/iter",
            "extra": "iterations: 1764972\ncpu: 393.7880566943845 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5528.561810538276,
            "unit": "ns/iter",
            "extra": "iterations: 126548\ncpu: 5528.481635426863 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5166.242419800615,
            "unit": "ns/iter",
            "extra": "iterations: 136441\ncpu: 5166.122880952207 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 379.9088457000688,
            "unit": "ns/iter",
            "extra": "iterations: 1845179\ncpu: 379.9048303714707 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5436.114373554117,
            "unit": "ns/iter",
            "extra": "iterations: 128806\ncpu: 5436.039594428843 ns\nthreads: 1"
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
          "id": "b00325e38be45acd8d618163ba11f7c3e667b968",
          "message": "Update build.yml",
          "timestamp": "2024-10-30T16:53:43-06:00",
          "tree_id": "cd2e997017b1e1fe0f3728187f769604a47daa5a",
          "url": "https://github.com/novatel/novatel_edie/commit/b00325e38be45acd8d618163ba11f7c3e667b968"
        },
        "date": 1730328948357,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 116.27126288949108,
            "unit": "ns/iter",
            "extra": "iterations: 5794062\ncpu: 116.2691015387823 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 119.6953599782752,
            "unit": "ns/iter",
            "extra": "iterations: 5627409\ncpu: 119.69322027242025 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 122.4362673303968,
            "unit": "ns/iter",
            "extra": "iterations: 5719649\ncpu: 122.42167587556503 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 116.57458531286628,
            "unit": "ns/iter",
            "extra": "iterations: 5773449\ncpu: 116.5738271871804 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146262.35886585063,
            "unit": "ns/iter",
            "extra": "iterations: 4726\ncpu: 146259.8614049938 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 404.8388443552879,
            "unit": "ns/iter",
            "extra": "iterations: 1733647\ncpu: 404.83337496041577 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5557.2417566631,
            "unit": "ns/iter",
            "extra": "iterations: 125283\ncpu: 5557.0404683795805 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5155.636664163184,
            "unit": "ns/iter",
            "extra": "iterations: 135822\ncpu: 5155.54413128948 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.78868750442206,
            "unit": "ns/iter",
            "extra": "iterations: 1862772\ncpu: 374.778591260766 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5441.582914455738,
            "unit": "ns/iter",
            "extra": "iterations: 128518\ncpu: 5441.18616847445 ns\nthreads: 1"
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
          "id": "b00325e38be45acd8d618163ba11f7c3e667b968",
          "message": "benchmarks",
          "timestamp": "2024-10-18T17:41:22Z",
          "url": "https://github.com/novatel/novatel_edie/pull/80/commits/b00325e38be45acd8d618163ba11f7c3e667b968"
        },
        "date": 1730328950047,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 124.34840158454529,
            "unit": "ns/iter",
            "extra": "iterations: 5948954\ncpu: 124.34455788362122 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 116.20996946922278,
            "unit": "ns/iter",
            "extra": "iterations: 5956285\ncpu: 116.20491917361235 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 118.06861299035242,
            "unit": "ns/iter",
            "extra": "iterations: 5698367\ncpu: 118.06422629500703 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 117.35726632146486,
            "unit": "ns/iter",
            "extra": "iterations: 5884291\ncpu: 117.35297013692895 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 147746.80079932805,
            "unit": "ns/iter",
            "extra": "iterations: 4754\ncpu: 147737.99852755573 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 402.6059805368774,
            "unit": "ns/iter",
            "extra": "iterations: 1744325\ncpu: 402.5885371132104 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5547.879018800743,
            "unit": "ns/iter",
            "extra": "iterations: 125846\ncpu: 5547.18131684758 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5180.487048826021,
            "unit": "ns/iter",
            "extra": "iterations: 134621\ncpu: 5180.210984913204 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 396.91643029669837,
            "unit": "ns/iter",
            "extra": "iterations: 1809663\ncpu: 396.8902845446915 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5417.156528879554,
            "unit": "ns/iter",
            "extra": "iterations: 129088\ncpu: 5416.550756073373 ns\nthreads: 1"
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
          "id": "ebb95e0ef55e877eed8f5059fbd8cb7e919654d0",
          "message": "Update build.yml",
          "timestamp": "2024-10-30T16:57:09-06:00",
          "tree_id": "298177310ae004a43a6657e872571f6b5d394c7a",
          "url": "https://github.com/novatel/novatel_edie/commit/ebb95e0ef55e877eed8f5059fbd8cb7e919654d0"
        },
        "date": 1730329211663,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 115.39401159224619,
            "unit": "ns/iter",
            "extra": "iterations: 6062847\ncpu: 115.39114528207627 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 114.79956459686902,
            "unit": "ns/iter",
            "extra": "iterations: 6101472\ncpu: 114.79880641917231 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 121.8066258578444,
            "unit": "ns/iter",
            "extra": "iterations: 6085612\ncpu: 121.80408478227 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 115.18047913343489,
            "unit": "ns/iter",
            "extra": "iterations: 6113704\ncpu: 115.17725032157267 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 145696.68364165668,
            "unit": "ns/iter",
            "extra": "iterations: 4811\ncpu: 145696.22552483893 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 392.86148704345806,
            "unit": "ns/iter",
            "extra": "iterations: 1783270\ncpu: 392.8559158175712 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5619.291722984018,
            "unit": "ns/iter",
            "extra": "iterations: 124296\ncpu: 5619.249123061072 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5184.143503189769,
            "unit": "ns/iter",
            "extra": "iterations: 136213\ncpu: 5183.93806024388 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.3264667848062,
            "unit": "ns/iter",
            "extra": "iterations: 1869838\ncpu: 374.3180409211915 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5572.183450985924,
            "unit": "ns/iter",
            "extra": "iterations: 126219\ncpu: 5571.963729707887 ns\nthreads: 1"
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
          "id": "7c325ab77b8b5715c432668ab7f7b6c4c456fd29",
          "message": "Revert \"Update build.yml\"\n\nThis reverts commit b00325e38be45acd8d618163ba11f7c3e667b968.",
          "timestamp": "2024-10-30T17:02:49-06:00",
          "tree_id": "5a470aba0ba6701c2c4a3d41a532821d71cf5721",
          "url": "https://github.com/novatel/novatel_edie/commit/7c325ab77b8b5715c432668ab7f7b6c4c456fd29"
        },
        "date": 1730329488347,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 130.4935498519723,
            "unit": "ns/iter",
            "extra": "iterations: 5630801\ncpu: 130.48980331572722 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 124.53768789749665,
            "unit": "ns/iter",
            "extra": "iterations: 5673572\ncpu: 124.53173573896655 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 129.6581916720494,
            "unit": "ns/iter",
            "extra": "iterations: 5577506\ncpu: 129.65566455688264 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 123.72833866043317,
            "unit": "ns/iter",
            "extra": "iterations: 5671002\ncpu: 123.7261025476626 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 148260.34431069222,
            "unit": "ns/iter",
            "extra": "iterations: 4403\ncpu: 148250.40699523053 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 394.6463523019355,
            "unit": "ns/iter",
            "extra": "iterations: 1781137\ncpu: 394.6277136458338 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5622.653631374419,
            "unit": "ns/iter",
            "extra": "iterations: 125352\ncpu: 5622.487961899287 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5174.70804921369,
            "unit": "ns/iter",
            "extra": "iterations: 135653\ncpu: 5174.4304954553245 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.5786996427041,
            "unit": "ns/iter",
            "extra": "iterations: 1869640\ncpu: 374.55635202498934 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5466.180293172148,
            "unit": "ns/iter",
            "extra": "iterations: 127570\ncpu: 5465.920169318804 ns\nthreads: 1"
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
          "id": "b028feb3f3d28fb62cc9f238c07157eab20cfdad",
          "message": "Update build.yml",
          "timestamp": "2024-10-30T17:31:22-06:00",
          "tree_id": "5a470aba0ba6701c2c4a3d41a532821d71cf5721",
          "url": "https://github.com/novatel/novatel_edie/commit/b028feb3f3d28fb62cc9f238c07157eab20cfdad"
        },
        "date": 1730331211194,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 116.4171975981522,
            "unit": "ns/iter",
            "extra": "iterations: 6132775\ncpu: 116.40553322109484 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 114.26547202923702,
            "unit": "ns/iter",
            "extra": "iterations: 6142003\ncpu: 114.26001110712586 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 121.19647892513414,
            "unit": "ns/iter",
            "extra": "iterations: 5957499\ncpu: 121.1935156010937 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 114.01831700082703,
            "unit": "ns/iter",
            "extra": "iterations: 6178577\ncpu: 114.01392812616888 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 146905.6520076465,
            "unit": "ns/iter",
            "extra": "iterations: 4707\ncpu: 146897.3960059486 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 398.2511520257198,
            "unit": "ns/iter",
            "extra": "iterations: 1775351\ncpu: 398.23336906335726 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5530.876291185417,
            "unit": "ns/iter",
            "extra": "iterations: 126531\ncpu: 5530.593190601518 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5180.074568086587,
            "unit": "ns/iter",
            "extra": "iterations: 135849\ncpu: 5180.002988612358 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.52130736778446,
            "unit": "ns/iter",
            "extra": "iterations: 1868579\ncpu: 374.510307565267 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 5447.88542708124,
            "unit": "ns/iter",
            "extra": "iterations: 129027\ncpu: 5447.9074999806235 ns\nthreads: 1"
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
          "id": "3c99fa41eddcc346376befc6a4935c650c8b594c",
          "message": "Update build.yml",
          "timestamp": "2024-10-30T17:37:14-06:00",
          "tree_id": "5a470aba0ba6701c2c4a3d41a532821d71cf5721",
          "url": "https://github.com/novatel/novatel_edie/commit/3c99fa41eddcc346376befc6a4935c650c8b594c"
        },
        "date": 1730331558990,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "DecodeFlattenedBinaryLog",
            "value": 120.37793076692687,
            "unit": "ns/iter",
            "extra": "iterations: 6067823\ncpu: 120.3767479704006 ns\nthreads: 1"
          },
          {
            "name": "DecodeAsciiLog",
            "value": 113.93114591060936,
            "unit": "ns/iter",
            "extra": "iterations: 6157688\ncpu: 113.92704583278658 ns\nthreads: 1"
          },
          {
            "name": "DecodeAbbrevAsciiLog",
            "value": 117.49723046861192,
            "unit": "ns/iter",
            "extra": "iterations: 5922121\ncpu: 117.49258956377291 ns\nthreads: 1"
          },
          {
            "name": "DecodeBinaryLog",
            "value": 119.0206795051803,
            "unit": "ns/iter",
            "extra": "iterations: 6077805\ncpu: 119.0155592356122 ns\nthreads: 1"
          },
          {
            "name": "DecodeJsonLog",
            "value": 147486.7650513997,
            "unit": "ns/iter",
            "extra": "iterations: 4767\ncpu: 147483.62555066083 ns\nthreads: 1"
          },
          {
            "name": "EncodeFlattenedBinaryLog",
            "value": 393.09382957737216,
            "unit": "ns/iter",
            "extra": "iterations: 1730627\ncpu: 393.0749127339402 ns\nthreads: 1"
          },
          {
            "name": "EncodeAsciiLog",
            "value": 5620.692748076225,
            "unit": "ns/iter",
            "extra": "iterations: 125415\ncpu: 5620.3703384762475 ns\nthreads: 1"
          },
          {
            "name": "EncodeAbbrevAsciiLog",
            "value": 5249.031998346977,
            "unit": "ns/iter",
            "extra": "iterations: 135507\ncpu: 5248.980746382114 ns\nthreads: 1"
          },
          {
            "name": "EncodeBinaryLog",
            "value": 374.7700927913532,
            "unit": "ns/iter",
            "extra": "iterations: 1867523\ncpu: 374.74739374026433 ns\nthreads: 1"
          },
          {
            "name": "EncodeJsonLog",
            "value": 6061.323188789335,
            "unit": "ns/iter",
            "extra": "iterations: 117173\ncpu: 6060.896708286046 ns\nthreads: 1"
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
          "id": "51b987ff46bdb1fc18be9bd0094e5c2837a226cd",
          "message": "Update benchmark.cpp",
          "timestamp": "2024-10-30T20:17:06-06:00",
          "tree_id": "f8706452c0a149051024eae618a4e9c9d590764c",
          "url": "https://github.com/novatel/novatel_edie/commit/51b987ff46bdb1fc18be9bd0094e5c2837a226cd"
        },
        "date": 1730341150169,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_DecodeFlattenedBinaryLog",
            "value": 117.56134060972107,
            "unit": "ns/iter",
            "extra": "iterations: 5729035\ncpu: 117.55230802395165 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAsciiLog",
            "value": 115.98881495734447,
            "unit": "ns/iter",
            "extra": "iterations: 6060862\ncpu: 115.97948724125376 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAbbrevAsciiLog",
            "value": 117.10530842569429,
            "unit": "ns/iter",
            "extra": "iterations: 5994734\ncpu: 117.10032838821537 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeBinaryLog",
            "value": 115.62992666249336,
            "unit": "ns/iter",
            "extra": "iterations: 6016294\ncpu: 115.62497444440051 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeJsonLog",
            "value": 146134.3720493007,
            "unit": "ns/iter",
            "extra": "iterations: 4787\ncpu: 146132.42636306648 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeFlattenedBinaryLog",
            "value": 393.55118156964267,
            "unit": "ns/iter",
            "extra": "iterations: 1769299\ncpu: 393.5258692849539 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAsciiLog",
            "value": 5554.31644947543,
            "unit": "ns/iter",
            "extra": "iterations: 126156\ncpu: 5553.965059133133 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAbbrevAsciiLog",
            "value": 5140.578870668574,
            "unit": "ns/iter",
            "extra": "iterations: 135868\ncpu: 5140.28184708688 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeBinaryLog",
            "value": 377.81419424175147,
            "unit": "ns/iter",
            "extra": "iterations: 1862854\ncpu: 377.8019474419356 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeJsonLog",
            "value": 5418.017623308617,
            "unit": "ns/iter",
            "extra": "iterations: 128296\ncpu: 5417.690668454192 ns\nthreads: 1"
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
          "id": "56f68d02ca89f0fb17aa114d9272e07de721957d",
          "message": "Update message_decoder.hpp",
          "timestamp": "2024-10-30T20:27:55-06:00",
          "tree_id": "524cf55af4649f87b0d96aae22fffb388b9a4860",
          "url": "https://github.com/novatel/novatel_edie/commit/56f68d02ca89f0fb17aa114d9272e07de721957d"
        },
        "date": 1730341796724,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_DecodeFlattenedBinaryLog",
            "value": 125.67568746536254,
            "unit": "ns/iter",
            "extra": "iterations: 5517878\ncpu: 125.65871735475123 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAsciiLog",
            "value": 121.4200990895914,
            "unit": "ns/iter",
            "extra": "iterations: 5869032\ncpu: 121.41780654799635 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAbbrevAsciiLog",
            "value": 125.30371728962547,
            "unit": "ns/iter",
            "extra": "iterations: 5567309\ncpu: 125.29716187838679 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeBinaryLog",
            "value": 136.7035923468546,
            "unit": "ns/iter",
            "extra": "iterations: 5302606\ncpu: 136.69648791556455 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeJsonLog",
            "value": 146683.2484342367,
            "unit": "ns/iter",
            "extra": "iterations: 4790\ncpu: 146681.40438413367 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeFlattenedBinaryLog",
            "value": 387.951137013692,
            "unit": "ns/iter",
            "extra": "iterations: 1804024\ncpu: 387.9262077444645 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAsciiLog",
            "value": 5736.014287577117,
            "unit": "ns/iter",
            "extra": "iterations: 122694\ncpu: 5736.008745333934 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAbbrevAsciiLog",
            "value": 5178.168826739391,
            "unit": "ns/iter",
            "extra": "iterations: 134872\ncpu: 5177.801597069818 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeBinaryLog",
            "value": 373.4814032317751,
            "unit": "ns/iter",
            "extra": "iterations: 1874815\ncpu: 373.47269143888786 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeJsonLog",
            "value": 5496.5381920939835,
            "unit": "ns/iter",
            "extra": "iterations: 127042\ncpu: 5496.379488673038 ns\nthreads: 1"
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
          "id": "2b859daa49264171381df247161626ab9f22d5c1",
          "message": "Revert \"Update message_decoder.hpp\"\n\nThis reverts commit 56f68d02ca89f0fb17aa114d9272e07de721957d.",
          "timestamp": "2024-10-30T20:28:06-06:00",
          "tree_id": "f8706452c0a149051024eae618a4e9c9d590764c",
          "url": "https://github.com/novatel/novatel_edie/commit/2b859daa49264171381df247161626ab9f22d5c1"
        },
        "date": 1730342649010,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_DecodeFlattenedBinaryLog",
            "value": 121.70923857334972,
            "unit": "ns/iter",
            "extra": "iterations: 5258366\ncpu: 121.70128287000185 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAsciiLog",
            "value": 121.50208964698334,
            "unit": "ns/iter",
            "extra": "iterations: 5790691\ncpu: 121.49851010872452 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeAbbrevAsciiLog",
            "value": 122.28609133387084,
            "unit": "ns/iter",
            "extra": "iterations: 5729090\ncpu: 122.27613285879616 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeBinaryLog",
            "value": 126.22780535679426,
            "unit": "ns/iter",
            "extra": "iterations: 5817023\ncpu: 126.21673319840748 ns\nthreads: 1"
          },
          {
            "name": "BM_DecodeJsonLog",
            "value": 144711.77800701524,
            "unit": "ns/iter",
            "extra": "iterations: 4847\ncpu: 144705.10068083357 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeFlattenedBinaryLog",
            "value": 395.1403101834046,
            "unit": "ns/iter",
            "extra": "iterations: 1771468\ncpu: 395.1220688152426 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAsciiLog",
            "value": 5593.8426972533425,
            "unit": "ns/iter",
            "extra": "iterations: 126069\ncpu: 5593.641783467783 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeAbbrevAsciiLog",
            "value": 5164.654432481742,
            "unit": "ns/iter",
            "extra": "iterations: 133097\ncpu: 5164.3883934273545 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeBinaryLog",
            "value": 376.2197899339224,
            "unit": "ns/iter",
            "extra": "iterations: 1846181\ncpu: 376.2071741611462 ns\nthreads: 1"
          },
          {
            "name": "BM_EncodeJsonLog",
            "value": 5430.953184195099,
            "unit": "ns/iter",
            "extra": "iterations: 127756\ncpu: 5430.683349509999 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}