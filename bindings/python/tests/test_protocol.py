"""Test protocol plugins."""

import getpass
import json
import os
import shutil
from pathlib import Path

import dlite
from dlite.protocol import (
    Protocol,
    load_path,
    save_path,
    is_archive, archive_names,
    archive_extract,
    archive_add,
)
from dlite.testutils import importcheck, raises

yaml = importcheck("yaml")
requests = importcheck("requests")
paramiko = importcheck("paramiko")


thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output" / "protocol"
indir = thisdir / "input"


# Test load_path() and save_path()
# --------------------------------
data1 = load_path(indir / "coll.json")
save_path(data1, outdir / "coll.json", overwrite=True)
assert is_archive(data1) is False

data2 = load_path(indir, exclude=r".*(~|\.bak)$")
save_path(data2, outdir / "indir", overwrite=True, include=r".*\.json$")
assert is_archive(data2) is True
assert "rdf.ttl" in archive_names(data2)
assert archive_extract(data2, "coll.json") == data1


# Test file plugin
# ----------------
# Test save/load files via the Protocol class
outfile = outdir / "hello.txt"
outfile.unlink(missing_ok=True)
pr = Protocol(protocol="file", location=outfile, options="mode=rw")
pr.save(b"hello world")
assert pr.load() == b"hello world"
assert pr.query() == "hello.txt"
pr.close()
with raises(dlite.DLiteIOError):  # double-close raises an DLiteIOError
    pr.close()

# Test loading with dlite.Instance.load()
b = dlite.Instance.load("file", "json", indir/"blob.json")
assert b.content.tolist() == [97, 98, 99]

# Test saving with dlite.Instance.save()
b.save("file", "json", outdir/"blob.json", "mode=w")

# Test load directory
# Since a directory plugin would be application-specific, we call
# Protocol explicitly here
with Protocol("file", indir/"subdir") as pr:
    data = pr.load()
data1 = archive_extract(data, "blob1.json")
data2 = archive_extract(data, "blob2.json")
b1 = dlite.Instance.from_bytes("json", data1)
b2 = dlite.Instance.from_bytes("json", data2)
assert b1.uri == "http://onto-ns.com/data/blob1"
assert b2.uri == "http://onto-ns.com/data/blob2"
assert b1.content.tolist() == [97, 98, 99]
assert b2.content.tolist() == [97, 98, 99]

# Test save directory
shutil.rmtree(outdir/"subdir", ignore_errors=True)
with Protocol("file", outdir/"subdir", options={"mode": "w"}) as pr:
    pr.save(data)
assert (outdir/"subdir"/"blob1.json").exists()
assert (outdir/"subdir"/"blob2.json").exists()
assert (outdir/"subdir"/"blob1.json").read_bytes() == data1
assert (outdir/"subdir"/"blob2.json").read_bytes() == data2


# Test http plugin
# ----------------
if requests:
    url = (
        "https://raw.githubusercontent.com/SINTEF/dlite/refs/heads/master/"
        "examples/entities/aa6060.json"
    )
    with Protocol(protocol="http", location=url) as pr:
        s = pr.load()
    d = json.loads(s)
    assert d["d4fe482c-ae2d-50f6-9dc8-ed64c5401bc6"]["uri"] == (
        "http://data.org/aa6060"
    )

if requests and yaml:
    url = (
        "https://raw.githubusercontent.com/SINTEF/dlite/refs/heads/master/"
        "storages/python/tests-python/input/test_meta.yaml"
    )
    m = dlite.Instance.load(protocol="http", driver="yaml", location=url)
    assert m.uri == "http://onto-ns.com/meta/0.1/TestEntity"
    assert type(m) is dlite.Metadata


# Test sftp plugin
# ----------------
if False:
    host = os.getenv("SFTP_HOST", "localhost")
    port = os.getenv("SFTP_PORT", "22")
    username = os.getenv("SFTP_USERNAME", getpass.getuser())
    password = os.getenv("SFTP_PASSWORD")  #
    pkpassword = os.getenv("SFTP_PKPASSWORD")  # Private key password
    keytype = os.getenv("SFTP_KEYTYPE", "ed25519")  # Private key type
    if paramiko and (password or pkpassword):
        options = {"port": int(port), "username": username}
        if pkpassword:
            pkey = paramiko.Ed25519Key.from_private_key_file(
                f"/home/{username}/.ssh/id_{keytype}",
                password=pkpassword,
            )
            options["pkey_type"] = pkey.name
            options["pkey_bytes"] = pkey.asbytes().hex()
        else:
            options["password"] = password

        b.save(
            "sftp", "json", location=f"{host}/tmp/blob.json", options=options
        )

    #v = con.load()
    #assert v.decode().startswith("[SemImageFile]")
    #
    #con = Protocol(
    #    protocol="sftp",
    #    location=f"{host}/P_MATCHMACKER_SHARE/SINTEF/test",
    #    options=f"port={port};username={username};password={password}",
    #)
    #uuid = dlite.get_uuid()
    #con.save(b"hello world", uuid=uuid)
    #val = con.load(uuid=uuid)
    #assert val.decode() == "hello world"
    #con.delete(uuid=uuid)
    #
    #if False:  # Don't polute sftp server
    #    con.save(data2, uuid="data2")
    #    data3 = con.load(uuid="data2")
    #    save_path(data3, outdir/"data3", overwrite=True)


# Test zip plugin
# ---------------
zipfile = indir / "subdir.zip"
zippath = "subdir/blob1.json"
location = f"{zipfile}#{zippath}"
blob1 = dlite.Instance.load(protocol="zip", driver="json", location=location)
assert blob1.uri == "http://onto-ns.com/data/blob1"

if requests:
    # Download blob1 from GitHub
    url1 = (
        "https://github.com/SINTEF/dlite/raw/refs/heads/zip-protocol/"
        "bindings/python/tests/input/subdir.zip#subdir/blob1.json"
    )
    blob = dlite.Instance.load(protocol="zip", driver="json", location=url1)
    assert blob.uri == "http://onto-ns.com/data/blob1"

    # Access blob2 from cache
    url2 = (
        "https://github.com/SINTEF/dlite/raw/refs/heads/zip-protocol/"
        "bindings/python/tests/input/subdir.zip#subdir/blob2.json"
    )
    blob2 = dlite.Instance.load(protocol="zip", driver="json", location=url2)
    assert blob2.uri == "http://onto-ns.com/data/blob2"

    # Use the protocol API directly
    pr = Protocol(protocol="zip", location=url2)
    data = pr.load()
    assert data.startswith(b'{\n  "f8e0')

    # Test Instance.from_url() against Zenodo
    zenodo_url = (
        "https://zenodo.org/record/1486184/files/github-mark.zip?download=1"
    )
    file_inside_zip = "github-mark.svg"
    pr = Protocol(protocol="zip", location=f"{zenodo_url}#{file_inside_zip}")
    data3 = pr.load()
    assert data3.startswith(b'<svg width="1024" height="1024" ')
