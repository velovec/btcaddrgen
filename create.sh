#!/bin/bash
docker build -t btcaddrgen:latest .
for i in {1..24}; do
  docker rm -f addrgen-${i}
done
docker rmi $(docker images -q -f dangling=true)

for i in {1..24}; do
  docker create --name addrgen-${i} -e "AMQP_HOST=195.201.109.37" btcaddrgen:latest
done
