"""Test the template storage plugin."""
from pathlib import Path

import dlite


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
dlite.storage_path.append(indir / "*.json")


inst = dlite.get_instance("http://data.org/my_test_instance_1")
inst.save("template", "test_template-format.out",
          options=f"template={indir}/template-format.txt")
