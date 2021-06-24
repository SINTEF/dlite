import os
import sys
import re
import subprocess

from pathlib import Path
from glob import glob
from sysconfig import get_path, get_config_var

import platform

from distutils import sysconfig, dir_util

from setuptools import setup, Extension, Distribution
from setuptools.command.build_ext import build_ext

# Based on
# https://github.com/galois-advertising/cmake_setup/blob/master/cmake_setup/cmake/__init__.py

SETUP_DIR = Path(__file__).parent.resolve()
SOURCE_DIR = (SETUP_DIR / "../.." ).resolve()
BUILD_DIR = SETUP_DIR / "mybuild"

CONFIG="Release"

BUILD_DIR.mkdir(exist_ok=True)

CMAKE_COMMON_VARIABLES = [
    #'-G', 'Visual Studio 15 2017',
    #'-A', 'x64',
    '-DWITH_DOC=OFF',
    '-DWITH_JSON=ON',
    '-DBUILD_JSON=ON',
    '-DWITH_HDF5=OFF',
    '-DWITH_PYTHON_BUILD_WHEEL=ON',
    #'-DCMAKE_BUILD_TYPE=%s' % config,
    #'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=%s' % extdir.parent.absolute(),
    #'-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=%s' % binary_dir,
    #f'-DCMAKE_INSTALL_PREFIX={install_dir}',
    #'-DALLOW_WARNINGS=ON',
    # Assume cmake version >= v3.12.4
    #'-DPython_EXECUTABLE=%s' % sys.executable,
    #'-DPYTHON_EXECUTABLE=%s' % sys.executable,
    #str(SOURCE_DIR)
]


class CMakeExtension(Extension):
    """
    setuptools.Extension for cmake
    """

    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuildExt(build_ext):
    """
    setuptools build_exit which builds using cmake & make
    You can add cmake args with the CMAKE_COMMON_VARIABLES environment variable
    """

    def build_extension(self, ext):
        if isinstance(ext, CMakeExtension):
            output_dir = os.path.abspath(
                os.path.dirname(self.get_ext_fullpath(ext.name)))

            build_type = 'Debug' if self.debug else 'Release'
            cmake_args = ['cmake',
                          str(SOURCE_DIR),
                          '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + output_dir,
                          '-DCMAKE_BUILD_TYPE=' + build_type,
                          ]
            cmake_args.extend(CMAKE_COMMON_VARIABLES)

            env = os.environ.copy()
            if not os.path.exists(self.build_temp):
                os.makedirs(self.build_temp)
            #self.build_temp=build\temp.win-amd64-3.8\Release
            #ext.sourcedir=C:\Users\ps-adm\repo\proj\A419409_OntoTRANS\SINTEF\dlite-github\api\python
            print(" ".join(cmake_args))
            subprocess.check_call(cmake_args,
                                  cwd=self.build_temp,
                                  env=env)
            #subprocess.check_call(['make', '-j', ext.name],
            #                      cwd=self.build_temp,
            #                      env=env)
        else:
            super().build_extension(ext)


requirements = [
    "numpy==1.19.2",
    "PyYAML",
    "psycopg2-binary",
    "pandas",
    "tables"
    ]

version = re.search(r'project\([^)]*VERSION\s+([0-9.]+)', Path(SOURCE_DIR / 'CMakeLists.txt').read_text()).groups()[0]

setup(
    name='dlite-python',
    version=version,
    author='SINTEF',
    author_email='jesper.friis@sintef.no',
    platforms=[ "Windows", "Linux" ],
    description=('Lightweight data-centric framework for working with '
                 'scientific data'),
    url='https://github.com/SINTEF/dlite',
    license='MIT',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        "Intended Audience :: Information Technology",
        "Intended Audience :: Science/Research",
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    #download_url=['https://github.com/SINTEF/dlite/archive/v0.2.5.tar.gz'],
    install_requires=requirements,

    #package_dir = { "dlite": r"../../out/build/x64-Release/bindings/python/dlite" }, # Must be relative
    #!package_dir = { "dlite": "packages/dlite" },

    #!packages= [ "dlite" ],
    #!package_data={ "dlite": [ dlite_compiled_ext, '*.dll', './share/dlite/storage-plugins/*dll'] },
    #!include_package_data=True,
    #!has_ext_modules=lambda: True,
    ext_modules=[CMakeExtension('make_target')],
    cmdclass={
        'build_ext': CMakeBuildExt,
    },
    #!distclass=BinaryDistribution,

    # FIXME: according to the setuptools documentation data_files is
    # deprecated and should be avoided since it doesn't work with weels.
    #
    # No alternative is mentioned in the documentation, though.  The
    # best I can think of is to use package_data and a post-install script
    # to move them to the right place.
    #data_files=[
    #    ('include/dlite', rglob('dist/include/dlite/**')),
    #    ('lib', rglob('dist/lib/**')),
    #    ('share/dlite', rglob('dist/share/**')),
    #    ('bin', glob('dist/bin/*')),
    #],
    zip_safe=False,
)