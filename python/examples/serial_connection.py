#!/usr/bin/env python3
########################################################################
#
# COPYRIGHT NovAtel Inc, 2022. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
########################################################################
#                            DESCRIPTION
#
# \file serial_connection.py
# \brief Demonstrates how to wrap the EDIE Parser to read data from a serial port.
########################################################################

import argparse
import logging
import platform
import serial
import serial.threaded
import time
from threading import Event

import novatel_edie as ne
import novatel_edie.messages as ne_msgs

from common_setup import setup_example_logging

class SerialParser:
    """A simple wrapper around the EDIE Parser to handle serial data.

    Wrappers like this one can help integrate the EDIE Parser with
    different data sources, such as serial ports, files, or network streams.

    An example of how to use this class is provided in the main function below.

    THIS CLASS IS AN EXAMPLE AND NOT PART OF THE PACKAGE'S PUBLIC INTERFACE.
    """
    def __init__(
            self,
            serial_connection: serial.Serial,
            message_db: ne.MessageDatabase = None):
        """Initializes the SerialParser.

        Args:
            serial_connection: A serial connection to read data from.
            message_db: Optional message database for parsing messages.
        """
        self._parser = ne.Parser(message_db)
        self._data_recieved = Event()
        self._serial_connection = serial_connection
        self._serial_reader = serial.threaded.ReaderThread(
            self._serial_connection,
            self._get_protocol(),
        )

    def start(self):
        """Starts the serial reader thread."""
        if not self._serial_connection.is_open:
            self._serial_connection.open()
        self._serial_reader.start()

    def stop(self):
        """Stops the serial parser and waits for it to shutdown."""
        self._serial_reader.stop()
        self._serial_reader.join()
        self._serial_connection.close()

    def read(self, timeout: float = 1) -> (
            ne.Message | ne.Response | ne.UnknownMessage| ne.UnknownBytes):
        """Reads data from the serial connection.

        Args:
            timeout: Maximum time to wait for data before throwing exception.

        Returns:
            A parsed message, response, unknown message, set of unknown bytes,
            or None if no data is available within the timeout.

        Raises:
            ne.BufferEmptyException: If no data is received within the timeout.
        """
        start_time = time.time()
        end_time = start_time + timeout
        while time.time() < end_time:
            try:
                return self._parser.read()
            except ne.BufferEmptyException:
                self._data_recieved.wait(end_time - time.time())
        raise ne.BufferEmptyException(
            f'No data received within {timeout} seconds.')

    def _get_protocol(self):
        """Creates a protocol that writes data to this SerialParser's parser.

        Returns:
            A protocol class that writes data to the EDIE parser.
        """
        class EdieProtocol(serial.threaded.Protocol):
            """A protocol that writes data to the EDIE parser."""
            def data_received(protocol_self, data):
                """Writes data to the SerialParser's parser and notifies it.

                Args:
                    data: The data received from the serial port.
                """
                self._parser.write(data)
                self._data_recieved.set()
                self._data_recieved.clear()

        return EdieProtocol


def normalize_serial_port(port: str) -> str:
    """Ensures correct port string format for Windows COM>9."""
    if platform.system() == 'Windows' and port.upper().startswith('COM') and int(port[3:]) > 9:
        return f'\\\\.\\{port}'
    return port


def parse_args():
    parser = argparse.ArgumentParser(description='Parse OEM logs from a serial port using the EDIE Parser.')
    parser.add_argument('--port', required=True, help='Serial port (e.g., /dev/ttyUSB0 or COM10)')
    parser.add_argument('--baudrate', type=int, default=9600, help='Baud rate of the serial connection')
    parser.add_argument('--timeout', type=float, default=2.0, help='Timeout for reading data from the serial port')
    parser.add_argument('--duration', type=float, default=None, help='Maximum run time in seconds')
    parser.add_argument('-V', '--version', action='store_true', help='Show EDIE version and exit')
    return parser.parse_args()


def main():
    """Uses a custom SerialParser class to read parsed messages from a serial port."""
    setup_example_logging(logging.INFO)
    logger = logging.getLogger(__name__)
    args = parse_args()
    if args.version:
        print(ne.CPP_PRETTY_VERSION)
        return

    # Handle arguments
    port = normalize_serial_port(args.port)
    baudrate = args.baudrate
    timeout = args.timeout
    duration = args.duration
    start_time = time.time()
    end_time = start_time + duration if duration is not None else None


    # Startup the connection
    messages = 0
    serial_connection = serial.Serial(port, baudrate, timeout=1)
    serial_parser = SerialParser(serial_connection)
    serial_parser.start()
    print(f'Listening on {port} at {baudrate} baud... Press Ctrl+C to stop.\n')
    try:
        while True:
            # Read data from the serial port and parse it
            try:
                message = serial_parser.read(timeout)
            except ne.BufferEmptyException:
                logger.warning('No data received within timeout. Retrying...')
                continue

            # Process the parsed message
            if isinstance(message, ne_msgs.BESTPOS):
                print(f'BESTPOS -> Lat: {message.latitude}, Lon: {message.longitude}')
                messages += 1
            elif isinstance(message, ne.UnknownMessage):
                pass
            elif isinstance(message, ne.UnknownBytes):
                pass

            # Check if program should stop
            if end_time and time.time() >= end_time:
                print(f'Duration limit reached ({duration:.1f} seconds).')
                break
    except KeyboardInterrupt:
        # Stop the serial parser and close the connection
        pass

    # Shutdown
    elapsed_seconds = time.time() - start_time
    logger.info(f'Parsed {messages} BESTPOS logs from serial port in {elapsed_seconds:.3f}s')
    print('Exiting.')
    serial_parser.stop()


if __name__ == '__main__':
    main()
