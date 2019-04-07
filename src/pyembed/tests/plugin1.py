import os
import dlite

# update storage search path
#path = os.path.abspath(os.path.join(
#    os.path.dirname(__file__), '..', '..', 'tests', 'mappings'))
#dlite.storage_plugin_path_append(path)

print("*** plugin1")
#for k, v in list(globals().items()):
#    print("%16s : %s" % (k, v))


class plugin1(DLiteMappingBase):
    name = "plugin1"
    output_uri = "http://meta.sintef.no/0.1/ent3"
    input_uris = ["http://meta.sintef.no/0.1/ent1"]
    cost = 25

    def map(self, instances):
        print('*** map(%r)' % (instances, ))
        inst1 = instances[0]
        inst3 = dlite.Instance(self.output_uri, [])
        inst3.c = inst1.a + 12
        return inst3
