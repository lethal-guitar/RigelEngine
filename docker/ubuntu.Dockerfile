FROM ubuntu:latest AS base

COPY docker/ubuntu-deps.sh /tmp/ubuntu-deps.sh

RUN ./tmp/ubuntu-deps.sh

RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 10 && \
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 20 && \
  update-alternatives --set c++ /usr/bin/g++ && \
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 10 && \
  update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 20 && \
  update-alternatives --set cc /usr/bin/gcc

RUN apt install -y gdb
RUN apt install -y clang-format-11

