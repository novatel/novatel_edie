from novatel_edie import hw_interface
from novatel_edie import novatel
import os

input_file = hw_interface.InputFileStream(os.path.join(os.path.dirname(os.path.realpath(__file__)), "resources/bestpos.bin"))
framer = novatel.Framer(input_file)

for log in framer:
    print(log.get_data_str())
    print(bytes(log.log_data[:log.length]))
