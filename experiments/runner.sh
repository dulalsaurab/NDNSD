#!/bin/bash

nfd-stop
sleep 1
nfd-start &> nfd.log
sleep 1

nfdc route add /discovery 262
sleep 1
nfdc route add /ndnsd 262
sleep 1

nfdc strategy set prefix /discovery strategy /localhost/nfd/strategy/multicast

export NDN_LOG=ndnsd.*=TRACE
ndnsd-consumer -s printer -c 1 -p 1 &> consumer.log

#ntpq -p >> consumer.log
