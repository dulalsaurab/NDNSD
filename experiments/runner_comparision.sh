mn -c
python ndnsd-experiments.py topologies/testbed_1.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/ndnsd/loss_1
sleep 10
mn -c

mn -c
python ndnsd-experiments.py topologies/testbed_2.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/ndnsd/loss_2
sleep 10

mn -c
mn -c
python proactive-experiment.py topologies/testbed_1.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/proactive/loss_1
sleep 10

mn -c
mn -c
python proactive-experiment.py topologies/testbed_2.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/proactive/loss_2
sleep 10

mn -c
mn -c
python reactive-experiment.py topologies/testbed_1.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/reactive/loss_1
sleep 10

mn -c
mn -c
python reactive-experiment.py topologies/testbed_2.conf
sleep 400
mv /tmp/minindn/* /home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/reactive/loss_2
sleep 10
