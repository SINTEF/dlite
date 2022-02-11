#!/bin/sh

set -e

docker build -f Dockerfile-python .

mkdir -p dist-docker
docker run --rm -it --volume $PWD/dist-docker:/packages/test bash
