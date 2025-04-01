from pathlib import Path

import dlite


# Add existing entities to storage path
thisdir = Path(__file__).resolve().parent
entitydir = thisdir / ".." / "entities"
dlite.storage_path.append(entitydir)

# URL for free MinIO playground
access_key = "Q3AM3UQ867SPQQA43P2F"
secret_key = "zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG"
url = f"minio://play.min.io?access_key={access_key};secret_key={secret_key}"


# Push to minio
aa6060 = dlite.get_instance("http://data.org/aa6060")
aa6082 = dlite.get_instance("http://data.org/aa6082")
with dlite.Storage(url) as s:
    s.save(aa6060.meta)  # Remember to also save metadata
    s.save(aa6060)
    s.save(aa6082)
