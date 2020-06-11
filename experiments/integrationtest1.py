import time
import sys

from mininet.log import setLogLevel, info

from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from subprocess import Popen, PIPE
import os
from minindn.apps.nlsr import Nlsr

import time
from random import randrange

numberOfUpdates = 100
jitter = 5

def cleanUp():
    pass

class NDNSDExperiment():

  def __init__(self, arg, producers, consumers):
    self.ndn = Minindn()
    self.args = arg
    self.producers = producers
    self.consumers = consumers
    self.producer_nodes = [host for host in ndn.net.hosts if host.name in self.producers]
    self.consumer_nodes = [host for host in ndn.net.hosts if host.name in self.consumers]
    self.start()

  def start(self):
    self.ndn.start()
    time.sleep(2)
    nfds = AppManager(self.ndn, self.ndn.net.hosts, Nfd)
    nlsrs = AppManager(self.ndn, self.ndn.net.hosts, Nlsr)
     # copy the test.info file to system first and from there copy it to node directory
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'],
           stdout=PIPE, stderr=PIPE).communicate()

  def startProducer(self):
    print("Starting producers")
    hostInfo = dict([])
    for host in self.producer_nodes: # if host.name not in consumer:
      hostName = host.name
      hostInfoFile = self.args.workDir+'/'+hostName+'/ndnsd_'+hostName+'.info'
      appPrefix = '/'+hostName+'/service-info'
      
      # hostInfo[host.name] = [hostInfoFile, appPrefix, '/printer']
      # appending filename just in case if we need in future
      # self.producers[hostName].append(hostInfoFile)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile],
             stdout=PIPE, stderr=PIPE).communicate()

      host.cmd('infoedit -f {} -s required.appPrefix -v {}'.format(hostInfoFile, appPrefix))
      
      cmd = 'nlsrc advertise {}'.format(appPrefix)
      host.cmd(cmd)
      
      cmd = 'nlsrc advertise {}{}'.format('/discovery/', self.producers[hostName][0])
      host.cmd(cmd)
      Nfdc.setStrategy(host, self.producers[hostName][0], Nfdc.STRATEGY_MULTICAST)

      cmd = 'export NDN_LOG=ndnsd.*=INFO'
      host.cmd(cmd)

      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      host.cmd(cmd)
      time.sleep(2)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumer_nodes:
      cName = consumer.name
      cmd = 'export NDN_LOG=ndnsd.*=INFO'
      consumer.cmd(cmd)

      # set multicast strategy for sync prefix
      Nfdc.setStrategy(consumer, self.producers[cName][0], Nfdc.STRATEGY_MULTICAST)

      cmd = 'ndnsd-consumer -s {} &> {}/{}/consumer.log -c 1 -p 1 &'.format(self.consumers[cName],
                                                                       self.args.workDir, cName)
      consumer.cmd(cmd)
      # sleep for a while to let consumer boot up properly


if __name__ == '__main__':
    setLogLevel('info')
    producers = dict()
    consumers = dict()
    producers['p'] = ['printer', 1000] #{"<sp-name>": ['service-name', 'lifetime in ms']}
    # producers['p2'] = ['printer', 1000]
    consumers= {'a':'printer', 'b':'printer', 'c':'printer','d':'printer', 'e':'printer'} #['<consumer-name>']

    ndn = Minindn()
    exp = NDNSDExperiment(ndn.args, producers, consumers)

    exp.startProducer()
    exp.startConsumer()

    # need to give some time for sync convergence
    time.sleep(500)

    # need to run reload at producers node
    for host in exp.producer_nodes:
      print("Staring experiment, i.e. reloading producer, approximate time to complete: {} seconds".format(numberOfUpdates+jitter))
      cmd = 'ndnsd-reload -c {} -i {} &> {}/{}/reload.log &'.format(numberOfUpdates,
                                                                    exp.producers[host.name][1]-10,
                                                                    ndn.args.workDir,
                                                                    host.name)
      host.cmd(cmd)

    # approximate time to complete the experiment
    time.sleep(2*numberOfUpdates + jitter)
    print("Experiment completed")

    ndn.stop()


# awk 'FNR==NR { array[$1]=$2; next } { if ($1 in array) print $1, array[$1] - $2 > "A3/f1.txt" }' ~/A1/f1.txt ~/A2/f1.txt


