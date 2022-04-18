#!/bin/bash

set -xe

BASE_URL="https://data.velovec.pro"

rm -f /bloom_data/bloom.metadata
for i in 1 3 bc; do
  META=$(curl -sSLf "${BASE_URL}/bloom_data.${i}.meta")

  MD5=$(echo "${META}" | awk '{ print $1 }')
  FILE_NAME=$(echo "${META}" | awk '{ print $2 }')
  FILE_SIZE=$(echo "${META}" | awk '{ print $3 }')

  curl -sSLf "${BASE_URL}/bloom_data.${i}" > "${FILE_NAME}"
  echo "${FILE_NAME}:${FILE_SIZE}" >> /bloom_data/bloom.metadata
done

NPROC=$(nproc)
for i in $(seq -f "%02g" 1 $NPROC); do
  supervisorctl start btcaddrgen:btcaddrgen_r${i} || echo "Skipped R${i}"
done