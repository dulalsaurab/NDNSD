import time
import os
import sys
import subprocess
from random import randrange
from subprocess import Popen, PIPE

from mininet.log import setLogLevel, info
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.apps.nlsr import Nlsr
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper

numberOfUpdates = 100
jitter = 100

def cleanUp():
    pass

def registerRouteToAllNeighbors(ndn, host, syncPrefix):
    for node in ndn.net.hosts:
        for neighbor in node.connectionsTo(host):
            ip = node.IP(neighbor[0])
            Nfdc.createFace(host, ip)
            Nfdc.registerRoute(host, syncPrefix, ip)

class NDNSDExperiment():
  def __init__(self, ndnobj, arg, producers, consumers):
    self.ndn = ndnobj
    self.args = arg
    self.producers = producers
    self.consumers = consumers
    self.producer_nodes = [host for host in ndn.net.hosts if host.name in self.producers]
    self.consumer_nodes = [host for host in ndn.net.hosts if host.name in self.consumers]
    self.start()

  def start(self):
    self.ndn.start()
    time.sleep(2)
    self.nfds = AppManager(self.ndn, self.ndn.net.hosts, Nfd, logLevel='DEBUG')
    self.nlsrs = AppManager(self.ndn, self.ndn.net.hosts, Nlsr, logLevel='DEBUG')
    time.sleep(100)
    # for host in self.ndn.net.hosts:
    #   host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 6363" -w {}.pcap &> /dev/null &'
    #            .format(host.name))
    #   #host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))
    #   time.sleep()
    # copy the test.info file to system first and from there copy it to node directory
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate()

  def startProducer(self):
    print("Starting producers")
    # hostInfo = dict([])
    for host in self.producer_nodes: # if host.name not in consumer:
      hostName = host.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, hostName, hostName)
      appPrefix = '/ndnsd/{}/service-info'.format(hostName)
      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile], stdout=PIPE, stderr=PIPE).communicate()
      host.cmd('infoedit -f {} -s required.appPrefix -v {}'.format(hostInfoFile, appPrefix))

      host.cmd('nlsrc advertise {}'.format("/discovery/printer"))
      host.cmd('nlsrc advertise {}'.format(appPrefix))
      time.sleep(2)

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=TRACE'
      host.cmd(cmd)
      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      host.cmd(cmd)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumer_nodes:
      cName = consumer.name
      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=TRACE'
      consumer.cmd(cmd)

      cmd = 'ndnsd-consumer -s {} -c 1 -p 1 &> {}/{}/consumer.log &'.format(self.consumers[cName], self.args.workDir, cName)
      consumer.cmd(cmd)

def partitionExperiment():
  pass

if __name__ == '__main__':
    setLogLevel('info')
    subprocess.call(['rm','-r','/tmp/minindn/'])

    producers = dict()
    consumers = dict()
    producers['p1'] = ['printer', 50]  #{"<sp-name>": ['service-name', 'lifetime in ms']}
    producers['p2'] = ['printer', 50]
    producers['p3'] = ['printer', 50]
    consumers = {'c1':'printer',  'c2':'printer'} # 'c3':'printer'} #['<consumer-name>']

    ndn = Minindn()
    exp = NDNSDExperiment(ndn, ndn.args, producers, consumers)
    exp.startProducer()
    exp.startConsumer()

    # set /discovery/printers to multicast on all the nodes.
    for host in ndn.net.hosts:
      Nfdc.setStrategy(host, '/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      time.sleep(1)

    # sync convergence
    time.sleep(30)

    for host in exp.producer_nodes:
      appPrefix = '/ndnsd/{}/service-info'.format(host.name)
      cmd = 'ndnsd-reload -c {} -i {} -r {} -p {} &> {}/{}/reload.log &'.format(numberOfUpdates, exp.producers[host.name][1] - 10,
                                                                                50, appPrefix, ndn.args.workDir, host.name)
      host.cmd(cmd)

    # approximate time to complete the experiment
    print("Sleep approximately {} seconds to complete the experiment".format(2*numberOfUpdates + jitter))
    time.sleep(100)
    print("Experiment completed")
    # ---------------------------------- setting up partition experiment end ----------------------------------

    MiniNDNCLI(ndn.net)
    ndn.stop()
