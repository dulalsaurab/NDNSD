import time
import sys

from mininet.log import setLogLevel, info

from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from subprocess import Popen, PIPE, call
import os
from minindn.apps.nlsr import Nlsr
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper
import subprocess

from time import sleep
from random import randrange

# from minindn.wifi.minindnwifi import MinindnWifi
from sys import exit
from traceback import format_exc
# from minindn.util import MiniNDNWifiCLI, getPopen

numberOfUpdates = 100
jitter = 20

def registerRouteToAllNeighbors(ndn, host, syncPrefix):
    for node in ndn.net.hosts:
        for neighbor in node.connectionsTo(host):
            ip = node.IP(neighbor[0])
            Nfdc.createFace(host, ip)
            Nfdc.registerRoute(host, syncPrefix, ip)

class NDNSDExperiment():
  def __init__(self, ndnwifi, producers, consumers):
    self.ndn = ndnwifi
    self.args = ndnwifi.args
    self.producers = producers
    self.consumers = consumers
    self.producer_nodes = [host for host in ndnwifi.net.hosts if host.name in self.producers]
    self.consumer_nodes = [host for host in ndnwifi.net.hosts if host.name in self.consumers]
    self.start()

  def setTsharkLog(self):
    for host in self.ndn.net.hosts:
      host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 6363" -w {}'.format(host.namey))
      # host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))
      time.sleep(0.1)

  def start(self):
    self.ndn.start()
    time.sleep(5)
    # self.setMTUsize(9000)
    nfds = AppManager(self.ndn, self.ndn.net.hosts, Nfd, logLevel='DEBUG')
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate()

  def setMTUsize(self, mtu):
    for host in self.ndn.net.hosts:
      for intf in host.intfList():
        host.cmd("ifconfig {} mtu {}".format(intf, mtu))

  def startProducer(self):
    print("Starting producers")
    hostInfo = dict()
    for host in self.producer_nodes: # if host.name not in consumer:
      hostName = host.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, hostName, hostName)
      appPrefix = '/ndnsd/{}/service-info'.format(hostName)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile], stdout=PIPE, stderr=PIPE).communicate()
      host.cmd('infoedit -f {} -s required.appPrefix -v {}'.format(hostInfoFile, appPrefix))

      # host.cmd('nlsrc advertise {}'.format("/discovery/printer"))
      # host.cmd('nlsrc advertise {}'.format(appPrefix))
      time.sleep(0.1)

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=NONE'
      host.cmd(cmd)
      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      host.cmd(cmd)
      time.sleep(3)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumer_nodes:
      cName = consumer.name
      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=TRACE'
      consumer.cmd(cmd)
      cmd = 'ndnsd-consumer -s {} &> {}/{}/consumer.log -c 1 -p 1 &'.format(self.consumers[cName], self.args.workDir, cName)
      consumer.cmd(cmd)
      # sleep for a while to let consumer boot up properly
      time.sleep(3)

# type = C or P for consumer and producer respectively
# count = how many?
def generateNode(type, count):
    nodes = dict()
    if type == 'C':
      for c in range(0, count):
        name = 'c{}'.format(c+1)
        nodes[name] = 'printer'
    elif type == 'P':
      for c in range(0, count):
        name = 'p{}'.format(c+1)
        nodes[name] = ['printer', 100]
    return nodes

if __name__ == '__main__':
    subprocess.call(['rm','-r','/tmp/minindn/*'])
    setLogLevel('info')
    ndn = Minindn()
    producers = dict()
    consumers = dict()
    producers = generateNode('P', 3)
    consumers = generateNode('C', 1)
    print(consumers, producers)
    exp = NDNSDExperiment(ndn, producers, consumers)

    # ----------------------------- adding static routes using ndn routing helper -----------------------
    # For all host, pass ndn.net.hosts or a list, [ndn.net['a'], ..] or [ndn.net.hosts[0],.]
    info('Adding static routes to NFD\n')
    grh = NdnRoutingHelper(ndn.net)
    for host in exp.producer_nodes:
      hostName = host.name
      appPrefix = '/ndnsd/{}/service-info'.format(hostName)
      discoveryPrefix = '/discovery/{}'.format(exp.producers[hostName][0])
      print ("Add routes for: ", hostName, appPrefix, discoveryPrefix)
      grh.addOrigin([host], [appPrefix, discoveryPrefix])
    grh.calculateNPossibleRoutes()
    # ----------------------------- adding static routes using ndn routing helper END-----------------------

    exp.startProducer()
    exp.startConsumer()
    time.sleep(10)
    # set /discovery/printers to multicast on all the nodes.
    for host in ndn.net.hosts:
      Nfdc.setStrategy(host, '/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      time.sleep(2)

    # need to run reload at producers node
    print("Staring experiment, i.e. reloading producers, approximate time to complete: {} seconds".format(2*(numberOfUpdates + jitter)))
    for host in exp.producer_nodes:
      appPrefix = '/ndnsd/{}/service-info'.format(host.name)
      cmd = 'ndnsd-reload -c {} -i {} -r {} -p {} &> {}/{}/reload.log &'.format(numberOfUpdates, exp.producers[host.name][1]-10,
                                                                                50, appPrefix, ndn.args.workDir, host.name)
      host.cmd(cmd)

    # approximate time to complete the experiment
    print("Sleep approximately {} seconds to complete the experiment".format(numberOfUpdates + jitter))
    time.sleep(150)
    print("Experiment completed")

    # Start the CLI
    # MiniNDNCLI(ndn.net)
    ndn.stop()

