from setuptools import setup, find_packages
from setuptools.command.test import test as test_command
import sys
import os
import glob
import shutil
import platform
from pathlib import Path

PACKAGE_NAME = 'novatel_edie'
VERSION_FILENAME = 'version.py'
DESCRIPTION = 'Package for working with NovAtel receiver data'
AUTHOR = 'rdoris'
AUTHOR_EMAIL = 'rdoris@novatel.com'
INSTALL_REQUIRES = []


'''AUTOMATICALLY GENERATED. DO NOT MODIFY ANYTHING BELOW THIS UNLESS YOU KNOW WHAT YOU ARE DOING'''

VERSION_PATH = os.path.join(os.path.dirname(__file__), PACKAGE_NAME, VERSION_FILENAME)

def copy_libraries(package_root, package_platform):
    Path("novatel_edie/resources").mkdir(parents=True, exist_ok=True)
    shutil.copy(os.path.join(package_root, 'database', 'novatel_log_definitions.json'), os.path.join('novatel_edie', 'resources'))
    bin_root = os.path.join(package_root, '\\bin')
    print(os.path.join(package_root, 'bin\\**\\Release-{}\\*.{}'.format(*package_platform)))
    package_so = glob.glob(os.path.join(package_root, 'bin\\**\\Release-{}\\*.{}'.format(*package_platform)), recursive=True)
    if not package_so:
        raise Exception('Unable to find the required .so/.dll files for EDIE')
    for file in package_so:
        fbase, fext = os.path.splitext(os.path.basename(file))
        if 'x64' in file:
            shutil.copyfile(file, os.path.join('novatel_edie', 'resources', '{}_x64{}'.format(fbase, fext)))
        else:
            shutil.copyfile(file, os.path.join('novatel_edie', 'resources', '{}_x32{}'.format(fbase, fext)))

        print('Copying "{}"'.format(file))

if os.environ.get('EDIE_ROOT'):
    edie_root = os.environ['EDIE_ROOT']
else:
    edie_root = os.path.normpath(os.path.join(os.getcwd(), '..\\..'))
print('EDIE_ROOT set to "{}"'.format(edie_root))

if not os.environ.get('EDIE_PLATFORM'):
    if sys.maxsize > 2**32:
        current_arch = 'x64'
    else:
        if platform.system() == 'Windows':
            current_arch = 'Win32'
        else:
            current_arch = 'x32'
        
    if platform.system() == 'Windows':
        package_platform = current_arch, 'dll'
    elif os.name == 'Linux':
        package_platform = current_arch, 'so'
    else:
        raise Exception('Platform "{}" is not currently supported'.format(platform.system()))
else:
    if os.environ['EDIE_PLATFORM'] == 'win32':
        package_platform = 'Win32', 'dll'
    elif os.environ['EDIE_PLATFORM'] == 'win64':
        package_platform = 'x64', 'dll'
    elif os.environ['EDIE_PLATFORM'] == 'linux32':
        package_platform = 'x32', 'so'
    elif os.environ['EDIE_PLATFORM'] == 'linux64':
        package_platform = 'x64', 'so'
    else:
        raise Exception('Platform "{}" is not currently supported'.format(platform.system()))

copy_libraries(edie_root, package_platform)

main_ns = {}
with open(VERSION_PATH) as ver_file:
    exec(ver_file.read(), main_ns)
VERSION = main_ns['__version__']


class PyTestCommand(test_command):
    user_options = [('pytest-args=', 'a', 'Arguments to pass to pytest')]

    def initialize_options(self):
      test_command.initialize_options(self)
      self.pytest_args = ''

    def run_tests(self):
      import shlex
      import pytest
      print(self.pytest_args)
      errno = pytest.main(shlex.split(self.pytest_args))
      sys.exit(errno)


setup(
    name=PACKAGE_NAME,
    version=VERSION,
    description=DESCRIPTION,
    long_description=open(os.path.join(os.path.dirname(__file__), 'readme.md')).read(),
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    packages=find_packages(exclude=['test', 'doc']),
    cmdclass={'test': PyTestCommand},
    tests_require=['pytest'],
    install_requires=INSTALL_REQUIRES,
    entry_points={},
    include_package_data=True,
    platforms=["Windows", "Linux"],
    package_data={
      'novatel_edie': [ 'novatel_edie/resources/*.dll',
                        'novatel_edie/resources/*.json'
                        ]
    },
    zip_safe=False,)
