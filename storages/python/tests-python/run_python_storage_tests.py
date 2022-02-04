import runpy
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
screen_width = shutil.get_terminal_size().columns - 1


def print_test_exception(err: Exception):
    print(''.center(screen_width, '#'))
    print('Exception raised:')
    print(str(err))
    print(''.center(screen_width, '#'))


if __name__ == '__main__':
    # The test scripts to run
    tests = [
        'test_bson_storage_python.py',
        'test_postgresql_storage_python.py',
        'test_yaml_storage_python.py',
        ]
    thisdir = Path('.')
    for t in tests:
        print(''.center(screen_width, '-'))
        try:
            runpy.run_path(thisdir / t)
        except Exception as err:
            print_test_exception(err)
