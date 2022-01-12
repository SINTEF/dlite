import runpy
import shutil
from pathlib import Path

screen_width = shutil.get_terminal_size().columns - 1


def print_test_exception(err: Exception):
    print(''.center(screen_width, '#'))
    print('Exception raised:')
    print(str(err))
    print(''.center(screen_width, '#'))


if __name__ == '__main__':
    # The test scripts to run
    tests = [
        'bson_test.py',
        'postgresql_test.py',
        'yaml_test.py',
        ]
    thisdir = Path('.')
    for t in tests:
        print(''.center(screen_width, '-'))
        try:
            runpy.run_path(thisdir / t)
        except Exception as err:
            print_test_exception(err)
