"""Test protocol plugins."""

import json
import os
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

paramiko = importcheck("paramiko")


thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output" / "protocol"
indir = thisdir / "input"


# Load plugins
#Protocol.load_plugins()


# Test load_path() and save_path()
# --------------------------------
data1 = load_path(indir / "coll.json")
save_path(data1, outdir / "coll.json", overwrite=True)
assert is_archive(data1) is False

data2 = load_path(indir, exclude=".*(~|\.bak)$")
save_path(data2, outdir / "indir", overwrite=True, include=".*\.json$")
assert is_archive(data2) is True
assert "rdf.ttl" in archive_names(data2)
assert archive_extract(data2, "coll.json") == data1


# Test file plugin
# ----------------
outfile = outdir / "hello.txt"
outfile.unlink(missing_ok=True)
pr = Protocol(protocol="file", location=outfile, options="mode=rw")
pr.save(b"hello world")
assert outfile.read_bytes() == b"hello world"
assert pr.load() == b"hello world"

assert pr.query() == "hello.txt"
pr.close()
with raises(dlite.DLiteIOError):
    pr.close()


# Test http plugin
# ----------------
url = "https://raw.githubusercontent.com/SINTEF/dlite/refs/heads/master/examples/entities/aa6060.json"
pr = Protocol(protocol="http", location=url)
s = pr.load()
d = json.loads(s)
assert d["25a1d213-15bb-5d46-9fcc-cbb3a6e0568e"]["uri"] == "aa6060"


# Test sftp plugin
# ----------------
host = os.getenv("AIMEN_SFTP_HOST")
port = os.getenv("AIMEN_SFTP_PORT")
username = os.getenv("AIMEN_SFTP_USERNAME")
password = os.getenv("AIMEN_SFTP_PASSWORD")
if paramiko and host and port and username and password:
    con = Protocol(
        protocol="sftp",
        location=(
            f"{host}/P_MATCHMACKER_SHARE/SINTEF/SEM_cement_batch2/"
            "77610-23-001/77610-23-001_15kV_400x_m008.txt"
        ),
        options=f"port={port};username={username};password={password}",
    )
    v = con.load()
    assert v.decode().startswith("[SemImageFile]")

    con = Protocol(
        protocol="sftp",
        location=f"{host}/P_MATCHMACKER_SHARE/SINTEF/test",
        options=f"port={port};username={username};password={password}",
    )
    uuid = dlite.get_uuid()
    con.save(b"hello world", uuid=uuid)
    val = con.load(uuid=uuid)
    assert val.decode() == "hello world"
    con.delete(uuid=uuid)

    if False:  # Don't polute sftp server
        con.save(data2, uuid="data2")
        data3 = con.load(uuid="data2")
        save_path(data3, outdir/"data3", overwrite=True)
