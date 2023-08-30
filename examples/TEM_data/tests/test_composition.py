import dlite

from paths import indir


compfile = indir / "composition.csv"

comp = dlite.Instance.from_location("composition", compfile)
print(comp)
