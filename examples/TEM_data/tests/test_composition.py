from paths import indir

import dlite


compfile = indir / "composition.csv"

comp = dlite.Instance.from_location("composition", compfile)
print(comp)

assert comp.alloy == "Al-Mg-Si"
assert comp.elements.tolist() == ['Al', 'Mg', 'Si', 'Mn', 'Fe']
assert comp.phases.tolist() == ['beta"']
assert comp.nominal_composition.tolist() == [97.88, 0.8, 0.8, 0.5, 0.02]
assert comp.phase_compositions.tolist() == [[0.1818, 0.4545, 0.3637, 0, 0]]
