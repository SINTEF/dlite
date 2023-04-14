import os
import sys
import platform
import re
import site
import subprocess
from typing import TYPE_CHECKING
from distutils import dir_util
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

if TYPE_CHECKING:
    from typing import Union

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
        "-DPython3_FIND_VIRTUALENV=ONLY",
        "-DPython3_FIND_IMPLEMENTATIONS=CPython",
    ]
    if not bool(int(os.getenv("CIBUILDWHEEL", "0"))):
        # Not running with `cibuildwheel`
        CMAKE_ARGS.extend(
            [
                f"-DPython3_EXECUTABLE={sys.executable}",
                "-DCMAKE_INSTALL_PREFIX="
                f"{site.USER_BASE if '--user' in sys.argv else sys.prefix}",
            ]
        )


elif platform.system() == "Windows":
    dlite_compiled_ext = "_dlite.pyd"
    dlite_compiled_dll_suffix = "*.dll"
    is_64bits = sys.maxsize > 2**32

    CMAKE_ARGS = [
        #"-G", "Visual Studio 15 2017",
        "-A", "x64",
        "-DWITH_DOC=OFF",
        "-DWITH_JSON=ON",
        "-DWITH_HDF5=OFF",
        "-Ddlite_PYTHON_BUILD_REDISTRIBUTABLE_PACKAGE=YES",
        f"-DCMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE={'x64' if is_64bits else 'x86'}"
    ]

else:
    raise NotImplementedError(f"Unsupported platform: {platform.system()}")


class CMakeExtension(Extension):
    """
    setuptools.Extension for cmake
    """

    def __init__(
        self,
        name: str,
        sourcedir: "Union[Path, str]",
        python_package_dir: "Union[Path, str]",
    ) -> None:
        """
        :param sourcedir: The root directory for the cmake build
        :param python_package_dir: The location of the Python package
            generated by CMake (relative to CMakes build-dir)
        """
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)
        self.python_package_dir = python_package_dir


class CMakeBuildExt(build_ext):
    """
    setuptools build_exit which builds using CMake
    """

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

    def build_extension(self, ext: CMakeExtension) -> None:
        """Run CMAKE

        Extra CMAKE arguments can be added through the `CI_BUILD_CMAKE_ARGS`
        environment variable.
        Note, the variables will be split according to the delimiter, which
        is a comma (`,`).

        Example:

            `CI_BUILD_CMAKE_ARGS="-DWITH_STATIC_PYTHON=YES,-DWITH_HDF5=NO"`
        """

        # The build_temp directory is not generated automatically on Windows,
        # generate it now
        Path(self.build_temp).mkdir(parents=True, exist_ok=True)

        output_dir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))

        environment_cmake_args = os.getenv("CI_BUILD_CMAKE_ARGS", "")
        environment_cmake_args = environment_cmake_args.split(",") if environment_cmake_args else []

        build_type = "Debug" if self.debug else "Release"
        cmake_args = [
            "cmake",
            str(ext.sourcedir),
            f"-DCMAKE_CONFIGURATION_TYPES:STRING={build_type}",
        ]
        cmake_args.extend(CMAKE_ARGS)
        cmake_args.extend(environment_cmake_args)

        env = os.environ.copy()

        try:
            subprocess.run(
                cmake_args,
                cwd=self.build_temp,
                env=env,
                capture_output=True,
                check=True)
        except subprocess.CalledProcessError as e:
            print("stdout:", e.stdout.decode("utf-8"), "\n\nstderr:",
                  e.stderr.decode("utf-8"))
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
            print("stdout:", e.stdout.decode("utf-8"), "\n\nstderr:",
                  e.stderr.decode("utf-8"))
            raise

        cmake_bdist_dir = Path(self.build_temp) / Path(ext.python_package_dir)
        dir_util.copy_tree(
            str(cmake_bdist_dir / ext.name), str(Path(output_dir) / ext.name)
        )

extra_requirements = [
    "fortran-language-server",
    "PyYAML",
    "psycopg2-binary==2.9.5",
    "pandas",
    "pymongo",
    "rdflib",
    "tripper",
    "pint",
]

requirements = ["numpy"]

version = re.search(
    r"project\([^)]*VERSION\s+([0-9.]+)",
    (SOURCE_DIR / "CMakeLists.txt").read_text(),
).groups()[0]

setup(
    name="DLite-Python",
    version=version,
    author="SINTEF",
    author_email="jesper.friis@sintef.no",
    platforms=["Windows", "Linux"],
    description=(
        "Lightweight data-centric framework for working with scientific data"
    ),
    long_description=(SOURCE_DIR / "README.md").read_text(encoding="utf-8"),
    long_description_content_type="text/markdown",
    url="https://github.com/SINTEF/dlite",
    license="MIT",
    python_requires=">=3.7",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
        "Operating System :: Microsoft :: Windows",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    install_requires=requirements + extra_requirements,
    # For now, the extra requirements are hard requirements.
    # See issue #222: https://github.com/SINTEF/dlite/issues/222
    # extras_require={"all": extra_requirements},
    packages=["dlite"],
    scripts=[
        str(SOURCE_DIR / "bindings" / "python" / "scripts" / "dlite-validate"),
    ],
    package_data={
        "dlite": [
            dlite_compiled_ext,
            dlite_compiled_dll_suffix,
            str(Path(".") / "share" / "dlite" / "storage-plugins" /
                dlite_compiled_dll_suffix),
            str(Path(".") / "bin" / "dlite-getuuid"),
            str(Path(".") / "bin" / "dlite-codegen"),
            str(Path(".") / "bin" / "dlite-env"),
        ]
    },
    ext_modules=[
        CMakeExtension(
            "dlite",
            sourcedir=SOURCE_DIR,
            python_package_dir=Path("bindings") / "python"
        )
    ],
    cmdclass={
        "build_ext": CMakeBuildExt,
    },
    zip_safe=False,
)
