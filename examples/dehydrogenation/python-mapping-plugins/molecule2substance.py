import dlite


class Molecule2Substance(DLiteMappingBase):
    name = 'Molecule2Substance'
    input_uris = ['http://onto-ns.com/meta/0.1/Molecule']
    output_uri = 'http://onto-ns.com/meta/0.1/Substance'
    cost = 25

    def map(self, instances):
        molecule = instances[0]
        substance = dlite.Instance.from_metaid(self.output_uri, [])
        substance.id = molecule.name
        substance.molecule_energy = molecule.groundstate_energy
        return substance
