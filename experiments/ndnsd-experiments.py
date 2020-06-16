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
jitter = 50

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
    nfds = AppManager(self.ndn, self.ndn.net.hosts, Nfd, logLevel='DEBUG')
    nlsrs = AppManager(self.ndn, self.ndn.net.hosts, Nlsr)

    for host in self.ndn.net.hosts:
      host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 6363" -w {}.pcap &> /dev/null &'
               .format(host.name))
      #host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))
      time.sleep(0.005)


    time.sleep(60)
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

      cmd = 'export NDN_LOG=ndnsd.*=DEBUG' #:psync.*=TRACE'
      host.cmd(cmd)

      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      host.cmd(cmd)
      time.sleep(10)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumer_nodes:
      cName = consumer.name

      cmd = 'export NDN_LOG=ndnsd.*=DEBUG' #:psync.*=TRACE'
      consumer.cmd(cmd)

      # set multi-cast strategy for sync prefix
      Nfdc.setStrategy(consumer, self.consumers[cName], Nfdc.STRATEGY_MULTICAST)

      cmd = 'ndnsd-consumer -s {} &> {}/{}/consumer.log -c 1 -p 1 &'.format(self.consumers[cName],
                                                                       self.args.workDir, cName)
      consumer.cmd(cmd)
      time.sleep(2)
      # sleep for a while to let consumer boot up properly

if __name__ == '__main__':
    setLogLevel('info')
    producers = dict()
    consumers = dict()
    producers['p1'] = ['printer', 1600] #{"<sp-name>": ['service-name', 'lifetime in ms']}
    producers['p2'] = ['printer', 1600]
    # consumers = {'a':'printer', 'b':'printer', 'c':'printer','d':'printer', 'e':'printer'} #['<consumer-name>']
    consumers = {'a':'printer', 'b':'printer'} #['<consumer-name>']

    ndn = Minindn()
    exp = NDNSDExperiment(ndn.args, producers, consumers)

    exp.startProducer()
    exp.startConsumer()

    # need to give some time for sync convergence
    time.sleep(20)

    # need to run reload at producers node
    for host in exp.producer_nodes:
      print("Staring experiment, i.e. reloading producer, approximate time to complete: {} seconds".format(numberOfUpdates+jitter))
      cmd = 'ndnsd-reload -c {} -i {} -r {} &> {}/{}/reload.log &'.format(numberOfUpdates,
                                                                    exp.producers[host.name][1]-10,
                                                                    300, ndn.args.workDir,
                                                                    host.name)
      host.cmd(cmd)

    # approximate time to complete the experiment
    print("sleep approximately {} seconds to complete the experiment".format(2*(numberOfUpdates + jitter)))
    time.sleep(3*(numberOfUpdates + jitter))
    print("Experiment completed")

    ndn.stop()

