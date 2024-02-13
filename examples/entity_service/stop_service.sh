#!/usr/bin/sh

# Start the dlite-entity-service in a docker container
rootdir="$(git rev-parse --show-toplevel)"
thisdir="${rootdir}/examples/entity_service"
servdir="${thisdir}/dlite-entities-service"


cd "$servdir"
docker-compose down
