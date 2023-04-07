#!/usr/bin/sh


set -e
rootdir="$(git rev-parse --show-toplevel)"
thisdir="${rootdir}/examples/entity_service"
servdir="${thisdir}/dlite-entities-service"


# Clone dlite-entity-service repo
if ! [ -d "$servdir" ]; then
    echo "Clone dlite-entities-service"
    cd "$thisdir"
    #git clone git@github.com:CasperWA/dlite-entities-service.git
    git clone https://github.com/CasperWA/dlite-entities-service.git
fi

# Start the dlite-entity-service in a docker container
regex='/[0-9a-f]* *dlite-entities-service_dlite_entities_service/p'
running="$(docker ps | sed -n "$regex")"
if [ -z "$running" ]; then
    echo "Start dlite-entities-service docker container"
    cd "$servdir"
    export MONGO_USER=$USER
    export MONGO_PASSWORD=$USER
    docker-compose pull
    docker-compose up --build --detach
fi
