FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get -qy install cmake make build-essential libboost-dev curl \
        libboost-program-options-dev libssl-dev libtool git

WORKDIR "/src"
RUN git clone https://github.com/bitcoin-core/secp256k1
WORKDIR "/src/secp256k1"
RUN ./autogen.sh && ./configure && make && make install

WORKDIR "/src"
RUN git clone https://github.com/velovec/ecdsa_cxx
WORKDIR "/src/ecdsa_cxx"
RUN mkdir build
WORKDIR "/src/ecdsa_cxx/build"
RUN cmake .. && make && make install

RUN ln -s /usr/local/lib/libsecp256k1.so.2 /lib/x86_64-linux-gnu/libsecp256k1.so.2
RUN mkdir /src/btcaddrgen

COPY . /src/btcaddrgen/
WORKDIR "/src/btcaddrgen"

RUN cmake . && make

ENTRYPOINT ["/src/btcaddrgen/btcaddrgen"]
