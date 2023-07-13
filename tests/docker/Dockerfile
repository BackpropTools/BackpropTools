FROM --platform=linux/amd64 nvidia/cuda:11.8.0-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# MKL
RUN apt-get update && apt-get install -y wget gnupg
RUN wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
| gpg --dearmor | tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null
RUN echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" | tee /etc/apt/sources.list.d/oneAPI.list
RUN apt-get update && apt-get install -y intel-oneapi-mkl-devel-2023.1.0

# Other dependencies
RUN apt-get update && apt-get install -y cmake build-essential git vim tmux libhdf5-dev libboost-all-dev protobuf-compiler libprotobuf-dev python3 python3-pip
RUN apt-get update && apt-get install -y libeigen3-dev

RUN git clone --recursive https://github.com/BackpropTools/BackpropTools.git
RUN mv BackpropTools /backprop_tools

RUN mkdir /build && mkdir /scripts
COPY configure_tests.sh /scripts/configure_tests.sh
COPY build_tests.sh /scripts/build_tests.sh


WORKDIR /build
RUN bash /scripts/configure_tests.sh
RUN bash /scripts/build_tests.sh
