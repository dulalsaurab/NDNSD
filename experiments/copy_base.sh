#!/usr/local/bin/bash

declare -A p
declare -A c

p["p1"]=192.168.0.192
p+=(["p2"]=192.168.0.219 ["p3"]=192.168.0.185)

c["c1"]=192.168.0.173
c+=(["c3"]=192.168.0.115 ["c4"]=192.168.0.171 ["c5"]=192.168.0.237 ["c6"]=192.168.0.220)

# c+=(["c2"]=192.168.0.226 ["c3"]=192.168.0.115 ["c4"]=192.168.0.171 ["c5"]=192.168.0.237 ["c6"]=192.168.0.220)


for key in ${!p[@]}; do
  mkdir $key
  echo pi@${p[${key}]}
  scp pi@${p[${key}]}:/home/pi/ndnsd-experiments/producer.log $key/
done

for key in ${!c[@]}; do
  mkdir $key
  scp pi@${c[${key}]}:/home/pi/ndnsd-experiments/consumer.log $key/
done
