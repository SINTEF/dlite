"""Test protocol plugins."""

import os
from pathlib import Path

import dlite
from dlite.protocol import Protocol, load_path, save_path


thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output"
indir = thisdir / "input"


if False:
    dlite.Protocol.load_plugins()

    host = "nas.aimen.es"
    port = 2122
    username = "matchmaker"
    password = os.getenv("AIMEN_SFTP_PASSWORD")

    conn = dlite.Protocol(
        scheme="sftp",
        location=f"{host}/dlitetest.txt",
        options=f"port={port};username={username};password={password}",
    )

    conn.save(b"hello world")

    assert conn.query() == "diitetest.txt"


# Test load_path() and save_path()
data1 = load_path(indir / "coll.json")
save_path(data1, outdir / "protocol" / "coll.json")

print("------- data2 --------")
data2 = load_path(indir, regex=".*[^~]$", zip_compression="none")
save_path(data2, outdir / "protocol" / "indir", overwrite=True)
