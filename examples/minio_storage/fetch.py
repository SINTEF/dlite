import dlite


# Add free MinIO playground to storage path
access_key = "Q3AM3UQ867SPQQA43P2F"
secret_key = "zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG"
url = f"minio://play.min.io?access_key={access_key};secret_key={secret_key}"
dlite.storage_path.append(url)


# Get data minio
aa6060 = dlite.get_instance("aa6060")
aa6082 = dlite.get_instance("aa6082")
