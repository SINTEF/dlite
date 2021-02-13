import os
import sys
import re
import pathlib
from glob import glob
from sysconfig import get_path, get_config_var

from distutils import sysconfig

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext as build_ext_orig


source_dir = pathlib.Path(__file__).parent.absolute()

# Path to dlite package installed by CMake
dlite_package = os.path.join(
    'dist',
    os.path.relpath(get_path('platlib'), get_config_var('exec_prefix')),
    'dlite')


def rglob(patt):
    """Recursive glob function that only returns ordinary files."""
    return [f for f in glob(patt, recursive=True) if os.path.isfile(f)]


class CMakeExtension(Extension):

    def __init__(self, name):
        # don't invoke the original build_ext for this special extension
        super().__init__(name, sources=[])


class build_ext(build_ext_orig):

    def run(self):
        for ext in self.extensions:
            self.build_cmake(ext)
        super().run()

    def build_cmake(self, ext):
        cwd = pathlib.Path().absolute()

        # these dirs will be created in build_py, so if you don't have
        # any python sources to bundle, the dirs will be missing
        binary_dir = pathlib.Path(self.build_temp)
        binary_dir.mkdir(parents=True, exist_ok=True)
        #extdir = pathlib.Path(self.get_ext_fullpath(ext.name))
        #extdir.mkdir(parents=True, exist_ok=True)

        # example of cmake args
        config = 'Debug' if self.debug else 'Release'
        cmake_args = [
            '-DCMAKE_BUILD_TYPE=%s' % config,
            #'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=%s' % extdir.parent.absolute(),
            #'-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=%s' % binary_dir,
            '-DCMAKE_INSTALL_PREFIX=%s/dist' % source_dir,
            '-DALLOW_WARNINGS=ON',
            # Assume cmake version >= v3.12.4
            #'-DPython_EXECUTABLE=%s' % sys.executable,
            '-DPYTHON_EXECUTABLE=%s' % sys.executable,
        ]

        # example of build args
        build_args = [
            '--config', config,
            '--', '-j4'
        ]

        os.chdir(str(binary_dir))
        self.spawn(['cmake', f'{source_dir}'] + cmake_args)
        if not self.dry_run:
            self.spawn(['cmake', '--build', '.'] + build_args)
            self.spawn(['cmake', '--install', '.'])

        # Troubleshooting: if fail on line above then delete all possible
        # temporary CMake files including "CMakeCache.txt" in top level dir.
        os.chdir(str(cwd))


# Extract current version from CMakeLists
cmakelists = pathlib.Path(source_dir / 'CMakeLists.txt')
version, = re.search(r'project\([^)]*VERSION\s+([0-9.]+)',
                    cmakelists.read_text()).groups()

setup(
    name='dlite',
    version=version,
    author='SINTEF',
    author_email='jesper.friis@sintef.no',
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
    packages=[''],
    package_data={'': rglob(dlite_package)},
    scripts=glob('dist/bin/*'),
    ext_modules=[CMakeExtension(os.path.join(dlite_package, '_dlite'))],
    data_files=[
        ('include/dlite', rglob('dist/include/dlite/**')),
        ('lib', rglob('dist/lib/**')),
        ('share/dlite', rglob('dist/share/**')),
    ],
    cmdclass={
        'build_ext': build_ext,
    }
)
