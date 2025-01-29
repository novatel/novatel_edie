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
import sys
from contextlib import contextmanager
import argparse
import subprocess

from novatel_edie_install_customizer.stubgen import StubGenerator

@contextmanager
def open_library_clone(library: str):
    """Creates a clone of specified files in a library and moves into the temp directory.

    Args:
        library: The name of the library to clone.
    """
    packages_path = os.path.join(sys.exec_prefix, 'Lib', 'site-packages')
    library_path = os.path.join(packages_path, library)
    if not os.path.exists(library_path):
        raise FileNotFoundError(f"Source library {library} not found.")
    library_dist_info = [
        d for d in os.listdir(packages_path)
        if d.startswith(library) and d.endswith('.dist-info')]
    if not library_dist_info:
        raise FileNotFoundError(
            f"No .dist-info directory found for library {library}.")
    library_dist_info = library_dist_info[0]
    library_dist_info_path = os.path.join(packages_path, library_dist_info)

    temp_dir = os.path.join(os.getcwd(), 'temp_dir')
    destination_dir = os.path.join(temp_dir, 'wheel')

    cwd = os.getcwd()

    try:
        if not os.path.exists(destination_dir):
            os.makedirs(destination_dir)
        shutil.copytree(
            library_path,
            os.path.join(destination_dir, library),
            dirs_exist_ok=True)
        shutil.copytree(
            library_dist_info_path,
            os.path.join(destination_dir, library_dist_info),
            dirs_exist_ok=True)

        os.chdir(temp_dir)
        yield
    finally:
        os.chdir(cwd)
        shutil.rmtree(temp_dir)

def copy_file(file_path: str, destination_path: str = None):
    """Copies a file to the current working directory.

    Args:
        file_path: The path of the file to copy.
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File {file_path} not found.")

    destination_path = (os.path.basename(file_path)
                        if destination_path is None
                        else destination_path)

    destination_path = os.path.join(os.getcwd(), destination_path)
    shutil.copy2(file_path, destination_path)

def install_package():
    """Installs a package in the current working directory.

    Args:
        package_name: The name of the package to install.
    """
    try:
        subprocess.check_call([
            sys.executable, '-m', 'wheel', 'pack', './wheel'])
        pass
    except subprocess.CalledProcessError as e:
        print(f"Failed to pack package: {e}")
    wheel_files = [f for f in os.listdir('.') if f.endswith('.whl')]
    for wheel_file in wheel_files:
        try:
            subprocess.check_call([
                sys.executable, '-m', 'pip',
                'install', wheel_file, '--force-reinstall'])
        except subprocess.CalledProcessError as e:
            print(f"Failed to install {wheel_file}: {e}")


def main(args):
    library_name = 'novatel_edie'
    with open_library_clone(library_name):
        copy_file(
            args.database, os.path.join('wheel', library_name, 'database.json'))

        database = StubGenerator(args.database)
        database.write_stub_files(os.path.join('wheel', library_name))

        install_package()



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some files.')
    parser.add_argument('database', type=str, help='The database to process')
    main(parser.parse_args())
