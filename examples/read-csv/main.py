try:
    import pandas  # noqa: F401
except ImportError:
    import sys

    sys.exit(77)  # skip this test if pandas is not available

import readcsv  # noqa: F401
