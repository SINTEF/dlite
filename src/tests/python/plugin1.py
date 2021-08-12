import os
import dlite

# update storage search path
#path = os.path.abspath(os.path.join(
#    os.path.dirname(__file__), '..', '..', 'tests', 'mappings'))
#dlite.storage_plugin_path_append(path)


class plugin1(DLiteMappingBase):
    name = "plugin1"
    output_uri = "http://onto-ns.com/meta/0.1/ent3"
    input_uris = ["http://onto-ns.com/meta/0.1/ent1"]
    cost = 25

    def map(self, instances):
        inst1 = instances[0]
        inst3 = dlite.Instance(self.output_uri, [])
        inst3.c = inst1.a + 12.0
        return inst3


class plugin2(DLiteMappingBase):
    name = "plugin2"
    output_uri = "http://onto-ns.com/meta/0.1/ent1"
    input_uris = ["http://onto-ns.com/meta/0.1/ent3"]
    cost = 25

    def map(self, instances):
        inst3 = instances[0]
        print(inst3)
        inst1 = dlite.Instance(self.output_uri, [])
        inst1.a = int(inst3.c - 12.0 + 0.5)
        return inst1
