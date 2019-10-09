import dlite

iter = dlite.StoragePluginIter()

s1 = dlite.Storage('json', 'test.json', 'mode=w')
s2 = dlite.Storage('hdf5', 'test.h5', 'mode=w')
s3 = dlite.Storage('yaml', 'test.yaml', 'mode=w')

inst = dlite.get_instance(dlite.ENTITY_SCHEMA)
inst.save(s3)


#name = iter.__next__()
#print(name)

#s = dlite.Storage('yaml', 'test.yaml', 'mode=w')
