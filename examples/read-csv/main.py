try:
    import pandas  # noqa: F401
    import tables  # noqa: F401
except ImportError:
    import sys
    sys.exit(44)  # skip this test if pandas is not available

import readcsv  # noqa: F401
