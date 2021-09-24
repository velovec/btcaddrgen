#!/bin/bash -xe
docker build -t btcaddrgen:latest .
for i in {0..31}; do
  docker rm -f addrgen-${i}
done
docker rmi $(docker images -q -f dangling=true)

for i in {0..31}; do
  from=$(( i * 8 ));
  to=$(( (i + 1) * 8 - 1 ))
  docker create --name addrgen-${i} -e "AMQP_HOST=195.201.109.37" -e "GENERATOR=direct" -e "FROM=${from}" -e "TO=${to}" btcaddrgen:latest
done
