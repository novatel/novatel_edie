import os
import logging
import argparse

from novatel_edie import string_to_encode_format, ENCODE_FORMAT

def setup_example_logging(log_level):
    root_logger = logging.getLogger()
    root_logger.setLevel(log_level)
    root_console_formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    root_console_handler = logging.StreamHandler()
    root_console_handler.setFormatter(root_console_formatter)
    root_console_handler.setLevel(log_level)
    root_logger.addHandler(root_console_handler)

def handle_args(logger) -> tuple[str, ENCODE_FORMAT]:
    """Parse and confirm accuracy of CLI arguments."""
    parser = argparse.ArgumentParser(description="Convert OEM log files using FileParser.")
    parser.add_argument("input_file", help="Input file")
    parser.add_argument("output_format", nargs="?",
                        choices=["ASCII", "ABBREV_ASCII", "BINARY", "FLATTENED_BINARY", "JSON"],
                        help="Output format", default="ASCII")
    parser.add_argument("-V", "--version", action="store_true")
    args = parser.parse_args()

    if args.version:
        exit(0)

    if not os.path.exists(args.input_file):
        logger.error(f'File "{args.input_file}" does not exist')
        exit(1)

    encode_format = string_to_encode_format(args.output_format)

    return args.input_file, encode_format
