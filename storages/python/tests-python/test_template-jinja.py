"""Test the template storage plugin."""
import sys
from pathlib import Path

import dlite

try:
    import jinja2
except ImportError:
    print("jinja2 not installed, skipping test jinja template")
    sys.exit(44)  # skip test


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
dlite.storage_path.append(indir / "*.json")


inst = dlite.get_instance("http://data.org/my_test_instance_1")
inst.save("template", "thttp://data.org/est_template-jinja.out",
          options=f"template={indir}/template-jinja.txt;engine=jinja")
