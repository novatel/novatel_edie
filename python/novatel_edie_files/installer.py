"""
MIT

Copyright (c) 2023 NovAtel Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

A module concerning the installation of a custom package.
"""

import os
import shutil
import importlib
import logging

import typer
from typing_extensions import Annotated

from novatel_edie.stubgen import StubGenerator

def install_custom(
        database: Annotated[
            str,
            typer.Argument(help='A path to a database file.')
        ]):
    """Create a custom installation of novatel_edie based on the provided DB.

    The custom installation will have all messages and enums of the provided
    database be directly importable from the 'enums' and 'messages' submodules.

    Args:
        database: A path to a database file.
    """
    # Determine directory information
    database = os.path.abspath(database)
    library_name = 'novatel_edie'
    lib_spec = importlib.util.find_spec(library_name)
    lib_dir = os.path.dirname(lib_spec.origin)
    database_path = os.path.join(lib_dir, 'database.json')

    # Update static type information
    logging.info('Updating static type information...')
    StubGenerator(database).write_stub_files(lib_dir)
    # Install new database
    logging.info('Copying database info...')
    try:
        shutil.copy2(database, database_path)
    except Exception as e:
        raise IOError('Failed to copy database info') from e
    logging.info(f'Database copied to {database_path}')

    logging.info(f'\nCustom installation of {library_name} created!\n')

if __name__ == '__main__':
    typer.run(install_custom)
