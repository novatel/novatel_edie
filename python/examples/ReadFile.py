from novatel_edie import stream_interface
import os

in_file = stream_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.asc"))

status, data = in_file.read()
print(data)

