import os
import sys
import platform
import re
import site
import subprocess
from distutils import dir_util
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

# Based on
# https://github.com/galois-advertising/cmake_setup/blob/master/cmake_setup/cmake/__init__.py

SETUP_DIR = Path(__file__).parent.resolve()
SOURCE_DIR = SETUP_DIR.parent

if platform.system() == "Linux":
    dlite_compiled_ext = "_dlite.so"
    dlite_compiled_dll_suffix = "*.so"

    CMAKE_ARGS = [
        "-DWITH_DOC=OFF",
        "-DWITH_JSON=ON",
        "-DWITH_HDF5=OFF",
        "-DALLOW_WARNINGS=ON",
        "-Ddlite_PYTHON_BUILD_REDISTRIBUTABLE_PACKAGE=YES",
        # Will always have CMake version >= 3.14 (see `CMakeLists.txt`)
        f"-DPython3_EXECUTABLE={sys.executable}",
        f"-DCMAKE_INSTALL_PREFIX={site.USER_BASE if '--user' in sys.argv else sys.prefix}",
        "-DPython3_FIND_VIRTUALENV=ONLY",
        "-DPython3_FIND_IMPLEMENTATIONS=CPython",
    ]

    if 'Python3_LIBRARY' in os.environ:
        CMAKE_ARGS.append(f"-DPython3_LIBRARY={os.environ['Python3_LIBRARY']}")
    if 'Python3_INCLUDE_DIR' in os.environ:
        CMAKE_ARGS.append(f"-DPython3_INCLUDE_DIR={os.environ['Python3_INCLUDE_DIR']}")
    if 'Python3_NumPy_INCLUDE_DIR' in os.environ:
        CMAKE_ARGS.append(f"-DPython3_NumPy_INCLUDE_DIR={os.environ['Python3_NumPy_INCLUDE_DIR']}")


elif platform.system() == "Windows":
    dlite_compiled_ext = "_dlite.pyd"
    dlite_compiled_dll_suffix = "*.dll"

    CMAKE_ARGS = [
        #"-G",
        #"Visual Studio 15 2017",
        "-A",
        "x64",
        "-DWITH_DOC=OFF",
        "-DWITH_JSON=ON",
        "-DWITH_HDF5=OFF",
        "-Ddlite_PYTHON_BUILD_REDISTRIBUTABLE_PACKAGE=YES",
    ]

else:
    raise NotImplementedError(f"Unsupported platform: {platform.system()}")


class CMakeExtension(Extension):
    """
    setuptools.Extension for cmake
    """

    def __init__(self, name, sourcedir, python_package_dir):
        """
        :param sourcedir: The root directory for the cmake build
        :param python_package_dir: The location of the Python package generated by CMake (relative to CMakes build-dir)
        """
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)
        self.python_package_dir = python_package_dir


class CMakeBuildExt(build_ext):
    """
    setuptools build_exit which builds using CMake
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def build_extension(self, ext):

        # The build_temp directory is not generated automatically on Windows, generate it now
        if not Path(self.build_temp).is_dir():
            Path(self.build_temp).mkdir(parents=True)

        output_dir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        build_type = "Debug" if self.debug else "Release"
        cmake_args = [
            "cmake",
            str(ext.sourcedir),
            f"-DCMAKE_CONFIGURATION_TYPES:STRING={build_type}",
        ]
        cmake_args.extend(CMAKE_ARGS)

        env = os.environ.copy()
        Path(self.build_temp).mkdir(exist_ok=True)

        try:
            subprocess.run(
                cmake_args,
                cwd=self.build_temp,
                env=env,
                capture_output=True,
                check=True)
        except subprocess.CalledProcessError as e:
            print("stdout:", e.stdout.decode("utf-8"), "\n\nstderr:", e.stderr.decode("utf-8"))
            raise
        try:
            subprocess.run(
                ["cmake", "--build", ".", "--config", build_type, "--verbose"],
                cwd=self.build_temp,
                env=env,
                capture_output=True,
                check=True
            )
        except subprocess.CalledProcessError as e:
            print("stdout:", e.stdout.decode("utf-8"), "\n\nstderr:", e.stderr.decode("utf-8"))
            raise

        cmake_bdist_dir = Path(self.build_temp) / Path(ext.python_package_dir)
        dir_util.copy_tree(
            str(cmake_bdist_dir / ext.name), str(Path(output_dir) / ext.name)
        )


requirements = [
    "fortran-language-server",
    "numpy",
    "PyYAML",
    "psycopg2-binary",
    "pandas",
    "pymongo",
]

setup_requirements = [
    "numpy"
]

version = re.search(
    r"project\([^)]*VERSION\s+([0-9.]+)",
    Path(SOURCE_DIR / "CMakeLists.txt").read_text(),
).groups()[0]

setup(
    name="DLite-Python",
    version=version,
    author="SINTEF",
    author_email="jesper.friis@sintef.no",
    platforms=["Windows", "Linux"],
    description=(
        "Lightweight data-centric framework for working with " "scientific data"
    ),
    url="https://github.com/SINTEF/dlite",
    license="MIT",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    # download_url=['https://github.com/SINTEF/dlite/archive/v0.2.5.tar.gz'],
    install_requires=requirements,
    setup_requires=setup_requirements,
    packages=["dlite"],
    package_data={
        "dlite": [
            dlite_compiled_ext,
            dlite_compiled_dll_suffix,
            "./share/dlite/storage-plugins/" + dlite_compiled_dll_suffix,
        ]
    },
    ext_modules=[
        CMakeExtension(
            "dlite", sourcedir=SOURCE_DIR, python_package_dir=Path("bindings/python")
        )
    ],
    cmdclass={
        "build_ext": CMakeBuildExt,
    },
    # FIXME: according to the setuptools documentation data_files is
    # deprecated and should be avoided since it doesn't work with weels.
    #
    # No alternative is mentioned in the documentation, though.  The
    # best I can think of is to use package_data and a post-install script
    # to move them to the right place.
    # data_files=[
    #    ('include/dlite', rglob('dist/include/dlite/**')),
    #    ('lib', rglob('dist/lib/**')),
    #    ('share/dlite', rglob('dist/share/**')),
    #    ('bin', glob('dist/bin/*')),
    # ],
    zip_safe=False,
)
