import dlite

print("*** load mapping plugin1")

class Person2SimplePerson(DLiteMappingBase):
    name = "Person2SimplePerson"
    output_uri = "http://onto-ns.com/meta/0.1/SimplePerson"
    input_uris = ["http://onto-ns.com/meta/0.1/Person"]
    cost = 25

    def map(self, instances):
        print("*** call map:", instances)
        person = instances[0]
        simple = dlite.Instance.from_metaid(self.output_uri, [])
        simple.name = person.name
        simple.age = person.age
        return simple


class SimplePerson2Person(DLiteMappingBase):
    name = "SimplePerson2Person"
    output_uri = "http://onto-ns.com/meta/0.1/Person"
    input_uris = ["http://onto-ns.com/meta/0.1/SimplePerson"]
    cost = 25

    def map(self, instances):
        print("*** call map 2:", instances)
        simple = instances[0]
        person = dlite.Instance.from_metaid(self.output_uri, [0])
        person.name = simple.name
        person.age = simple.age
        return person
