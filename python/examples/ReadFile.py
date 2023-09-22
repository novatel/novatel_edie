from novatel_edie import hw_interface
import os

in_file = hw_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.asc"))

status, data = in_file.read()
print(data)

