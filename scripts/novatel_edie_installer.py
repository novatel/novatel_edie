import os
import shutil
import sys
from contextlib import contextmanager
import argparse
import subprocess
import toml

@contextmanager
def open_library_clone(library: str, relative_filepaths: list[str]):
    """Creates a clone of specified files in a library and moves into the temp directory.

    Args:
        library: The name of the library to clone.
        relative_filepaths: A list of relative filepaths to clone.
    """
    library_path = os.path.join(sys.exec_prefix, 'Lib', 'site-packages', library)
    temp_dir = os.path.join(os.getcwd(), 'temp_dir')
    destination_directory = os.path.join(temp_dir, library)
    if not os.path.exists(library_path):
        raise FileNotFoundError(f"Source library {library} not found.")

    if not os.path.exists(destination_directory):
        os.makedirs(destination_directory)

    cwd = os.getcwd()

    try:
        for rel_path in relative_filepaths:
            s = os.path.join(library_path, rel_path)
            if not os.path.exists(s):
                raise FileNotFoundError(f"File {rel_path} not found in source library {library}.")
            d = os.path.join(destination_directory, rel_path)
            shutil.copy2(s, d)

        os.chdir(temp_dir)
        yield
    finally:
        os.chdir(cwd)
        shutil.rmtree(temp_dir)

def copy_file_to_cwd(file_path: str):
    """Copies a file to the current working directory.

    Args:
        file_path: The path of the file to copy.
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File {file_path} not found.")

    destination_path = os.path.join(os.getcwd(), os.path.basename(file_path))
    shutil.copy2(file_path, destination_path)

def install_package():
    """Installs a package in the current working directory.

    Args:
        package_name: The name of the package to install.
    """
    try:
        subprocess.check_call([sys.executable, '-m', 'pip', 'install', '.'])
    except subprocess.CalledProcessError as e:
        print(f"Failed to install package: {e}")

def create_pyproject_toml(package_name: str, version: str, description: str, author: str, author_email: str, license_str: str = 'MIT'):
    """Creates a basic pyproject.toml file.

    Args:
        package_name: The name of the package.
        version: The version of the package.
        description: A short description of the package.
        author: The name of the author.
        author_email: The email of the author.
    """
    pyproject_content = {
        'tool': {
            'poetry': {
                'name': package_name,
                'version': version,
                'description': description,
                'authors': [f"{author} <{author_email}>"],
                'license': license_str
            }
        },
        'build-system': {
            'requires': ['poetry-core>=1.0.0'],
            'build-backend': 'poetry.core.masonry.api'
        }
    }

    with open('pyproject.toml', 'w') as f:
        toml.dump(pyproject_content, f)


def main(args):
    library_name = 'novatel_edie'
    relative_filepaths = ['__init__.py', 'bindings.pyd']
    with open_library_clone(library_name, relative_filepaths):
        copy_file_to_cwd(args.database)
        create_pyproject_toml('novatel_edie', '0.1.0', 'A package for decoding NovAtel EDIE messages', 'Author', 'MIT')
        install_package()



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some files.')
    parser.add_argument('database', type=str, help='The database to process')
    main(parser.parse_args())
