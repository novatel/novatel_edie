## How to install
```pip install novatel_edie```

## EDIE Examples

#### Decoding logs
```
from novatel_edie import novatel
from novatel_edie import hw_interface
import os

print(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources\\bestpos.bin"))
in_file = hw_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.bin"))
decoder = novatel.Decoder(in_file)

for log in decoder:
    print(f'{log.header["MessageTime"]} {log.header["MessageName"]} - {log.body["eMyPositionStatus"]} '
          f'{log.body["dMyLatitude"]} {log.body["dMyLongitude"]} {log.body["fMyLatStdDev"]} {log.body["fMyLongStdDev"]} '
          f'{log.body["ucMyNumInSolution"]}')

```

#### Framing logs
```
from novatel_edie import hw_interface
from novatel_edie import novatel
import os

input_file = hw_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.bin"))
framer = novatel.Framer(input_file)

for log in framer:
    print(log.get_data_str())
    print(bytes(log.log_data[:log.length]))

```

## Roadmap
* Create a linux release of the package
* Create stream interfaces in the DLL
