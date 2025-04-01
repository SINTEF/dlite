import dlite


# Add free MinIO playground to storage path
access_key = "Q3AM3UQ867SPQQA43P2F"
secret_key = "zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG"
url = f"minio://play.min.io?access_key={access_key};secret_key={secret_key}"
dlite.storage_path.append(url)


# Get data from MinIO
aa6060 = dlite.get_instance("http://data.org/aa6060")
aa6082 = dlite.get_instance("http://data.org/aa6082")


# Show an instance we fetched
print(aa6082)


# Cleanup (to be nice and not leave our data laying around in the MinIO playground)
with dlite.Storage(url) as s:
    s.delete(aa6060.meta.uuid)
    s.delete(aa6060.uuid)
    s.delete(aa6082.uuid)
