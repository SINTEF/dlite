"""setup.py file for installing scripts."""
import re
import sys
from setuptools import setup
from glob import glob


with open('../../../CMakeLists.txt', 'rt') as f:
    match = re.search(r'VERSION +([0-9.]+)', f.read())
    version, = match.groups()


setup(
    name='dlite',
    version=version,
    scripts=['dlite-validate'],
    entry_points={
        'console_scripts': [
            'pip=pip:main',
            f'pip-{sys.version[:3]}=pip:main',
        ],
    },
)
