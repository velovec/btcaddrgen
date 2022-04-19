FROM docker.velovec.pro/btc/supervisor-base:latest

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

RUN ln -s /usr/local/lib/libsecp256k1.so.0.0.0 /lib/x86_64-linux-gnu/libsecp256k1.so.0
RUN mkdir /src/btcaddrgen

WORKDIR "/src"
RUN git clone https://github.com/velovec/libbloom
WORKDIR "/src/libbloom"
RUN make

COPY supervisor/btcaddrgen.conf /etc/supervisor/conf.d/btcaddrgen.conf

COPY . /src/btcaddrgen/
WORKDIR "/src/btcaddrgen"

RUN cmake . && make && chmod +x /src/btcaddrgen/btcaddrgen.sh

VOLUME ["/bloom_data"]