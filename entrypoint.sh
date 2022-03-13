#!/bin/bash

set -e

BASE_URL="https://data.velovec.pro"

function download_file {
   curl -s -o "${1}" "${BASE_URL}/${1}"
}

download_file bloom.sig
download_file bloom.metadata
download_file bloom-1.dat
download_file bloom-3.dat
download_file bloom-bc.dat

md5sum -c bloom.sig

/src/btcaddrgen/btcaddrgen