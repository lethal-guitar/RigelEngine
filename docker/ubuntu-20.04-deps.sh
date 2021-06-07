#!/bin/bash

set -ex

apt-get update && \
DEBIAN_FRONTEND="noninteractive" apt-get install --no-install-recommends -y \
    cmake \
    g++-8 \
    libboost-dev \
    libboost-program-options-dev \
    libsdl2-dev \
    libsdl2-mixer-dev \
    make
