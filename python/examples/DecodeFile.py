from novatel_edie import novatel
from novatel_edie import stream_interface
import os

print(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources\\bestpos.bin"))
in_file = stream_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.bin"))
decoder = novatel.Decoder(in_file)

for log in decoder:
    print(f'{log.header["MessageTime"]} {log.header["MessageName"]} - {log.body["eMyPositionStatus"]} '
          f'{log.body["dMyLatitude"]} {log.body["dMyLongitude"]} {log.body["fMyLatStdDev"]} {log.body["fMyLongStdDev"]} '
          f'{log.body["ucMyNumInSolution"]}')
