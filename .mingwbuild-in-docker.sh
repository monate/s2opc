#!/bin/bash
#
#  Run the script given parameters in the docker
#
set -e

DOCKER_IMAGE=f7d86fcc79d5

if [[ -z $SOPC_DOCKER_NEEDS_SUDO ]]; then
    /etc/scripts/run-in-docker $DOCKER_IMAGE CMAKE_TOOLCHAIN_FILE=toolchain-mingw32-w64.cmake BUILD_SHARED_LIBS=true "$@"
else
    sudo /etc/scripts/run-in-docker $DOCKER_IMAGE CMAKE_TOOLCHAIN_FILE=toolchain-mingw32-w64.cmake BUILD_SHARED_LIBS=true "$@"
fi
