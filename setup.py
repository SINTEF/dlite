import os
import platform
import subprocess
import sys
import re

from pathlib import Path
from glob import glob
from sysconfig import get_path, get_config_var


from distutils import sysconfig, dir_util
import pkg_resources

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# Based on
# https://github.com/galois-advertising/cmake_setup/blob/master/cmake_setup/cmake/__init__.py

SETUP_DIR = Path(__file__).parent.resolve()
SOURCE_DIR = SETUP_DIR


if platform.system() == "Linux":
    dlite_compiled_ext = "_dlite.so"
    dlite_compiled_dll_suffix = '*.so'

elif platform.system() == "Windows":
    dlite_compiled_ext = "_dlite.pyd"
    dlite_compiled_dll_suffix = '*.dll'

    CMAKE_COMMON_VARIABLES = [
        '-G', 'Visual Studio 15 2017',
        '-A', 'x64',
        '-DWITH_DOC=OFF',
        '-DWITH_JSON=ON',
        '-DBUILD_JSON=ON',
        '-DWITH_HDF5=OFF',
        '-DWITH_PYTHON_BUILD_WHEEL=ON',
        #'-DALLOW_WARNINGS=ON',
        # Assume cmake version >= v3.12.4
        #'-DPython_EXECUTABLE=%s' % sys.executable,
        #'-DPYTHON_EXECUTABLE=%s' % sys.executable,
    ]
    
else:
    raise NotImplementedError()


class CMakeExtension(Extension):
    """
    setuptools.Extension for cmake
    """

    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuildExt(build_ext):
    """
    setuptools build_exit which builds using cmake
    """

    def build_extension(self, ext):
        output_dir = os.path.abspath(
            os.path.dirname(self.get_ext_fullpath(ext.name)))

        build_type = 'Debug' if self.debug else 'Release'
        cmake_args = ['cmake',
                       ext.sourcedir,
                       f'-DCMAKE_CONFIGURATION_TYPES:STRING={build_type}',
                      ]
        cmake_args.extend(CMAKE_COMMON_VARIABLES)

        env = os.environ.copy()
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        print("Cmake>\n"+" ".join(cmake_args)+"\n")
        subprocess.check_call(cmake_args,
                                cwd=self.build_temp,
                                env=env)
        subprocess.check_call(['cmake', '--build', '.', '--config', build_type],
                                cwd=self.build_temp,
                                env=env)
        
        # TODO: Would be better to define a custom CMake target and install this to our self.build_temp/dlite
        cmake_bdist_dir=Path(self.build_temp) / Path(f"bindings/python")
        dir_util.copy_tree(str(cmake_bdist_dir / ext.name), str(Path(output_dir) / ext.name))
        
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

    packages = [ "dlite" ],
    package_data={ "dlite": [
        dlite_compiled_ext,
        dlite_compiled_dll_suffix,
        './share/dlite/storage-plugins/'+dlite_compiled_dll_suffix
        ]
    },
    ext_modules=[CMakeExtension('dlite', sourcedir=SOURCE_DIR)],
    cmdclass={
        'build_ext': CMakeBuildExt,
    },

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