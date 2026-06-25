#!/bin/bash

NAME=themiify
IMAGE=${NAME}-image
CONTAINER=${NAME}-container

cleanup()
{
    echo "Cleaning up Docker..."
    if test x${CONTAINER} != x
    then
        docker container rm --force ${CONTAINER}
    fi
    if test x${IMAGE} != x
    then
        docker image rm --force ${IMAGE}
    fi
    echo "You may also want to run 'docker system prune --force' to delete Docker's caches."
    exit $1
}
trap cleanup INT TERM

docker build --tag ${IMAGE} . || cleanup 1

ARGS="--tty --interactive --name ${CONTAINER} ${IMAGE}"

docker run ${ARGS} sh -c "powerpc-eabi-cmake -S . -B build && cmake --build build" || cleanup 2

echo "Compilation finished."

# Copy the output file out.
docker cp "${CONTAINER}:/project/build/${NAME}.wuhb" .

cleanup 0
