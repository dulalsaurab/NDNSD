import sys
import os
from time import sleep
from random import randrange
from sys import exit
from subprocess import Popen, PIPE, call
import subprocess
from traceback import format_exc

from mininet.log import setLogLevel, info
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.apps.nlsr import Nlsr
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen

def registerRouteToAllNeighbors(ndn, host, prefix):
    for node in ndn.net.hosts:
        for neighbor in node.connectionsTo(host):
            ip = node.IP(neighbor[0])
            Nfdc.createFace(host, ip)
            Nfdc.registerRoute(host, prefix, ip)

def setTsharkLog(ndn):
    for host in ndn.net.hosts:
        print ("Setting tshark for host:", host.name)
        host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 6363" -w {}.pcap &> /dev/null &'.format(host.name))
        # host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))
        sleep(0.1)

def setMTUsize(self, ndn, mtu=9000):
    for host in self.ndn.net.hosts:
        for intf in host.intfList():
            host.cmd("ifconfig {} mtu {}".format(intf, mtu))

# type = C or P for consumer and producer respectively
# count = how many?
def generateNodes(type, count, serviceType = 'printer', publicationInterval=100):
    nodes = dict()
    if type == 'C':
      for c in range(0, count):
        name = 'c{}'.format(c+1)
        nodes[name] = serviceType
    elif type == 'P':
      for c in range(0, count):
        name = 'p{}'.format(c+1)
        nodes[name] = [serviceType, publicationInterval]
    return nodes

class NDNSDExperiment():
  '''
    This is a base class for all ndnsd experiments (both wireless and wired)
      ndn: object, either mini-ndn object, can be minindn or minindnwifi object
      producers: list, service publishers
      consumers: list, service finder
      expType: string, wired or wireless
  '''
  def __init__(self, ndn, producers, consumers, expType="wired", nlsr=True):
    self.ndn = ndn
    self.args = ndn.args
    self.expType = expType
    self.producers = producers
    self.consumers = consumers
    if expType == 'wifi':
        self.hosts = ndn.net.stations
    else:
        self.hosts = ndn.net.hosts
    self.producerNodes = [host for host in self.hosts if host.name in self.producers]
    self.consumerNodes = [host for host in self.hosts if host.name in self.consumers]
    self.start(nlsr)

  def start(self, nlsr):
    self.ndn.start()
    sleep(5)
    AppManager(self.ndn, self.hosts, Nfd, logLevel='DEBUG')
    if nlsr:
        AppManager(self.ndn, self.ndn.net.hosts, Nlsr, logLevel='INFO')
        sleep(50)
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate()

  def startProducer(self):
    print("Starting producers")
    hostInfo = dict([])
    for producer in self.producerNodes: # if host.name not in consumer:
      hostName = producer.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, hostName, hostName)
      appPrefix = '/ndnsd/{}/service-info'.format(hostName)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile], stdout=PIPE, stderr=PIPE).communicate()
      producer.cmd('infoedit -f {} -s required.appPrefix -v {}'.format(hostInfoFile, appPrefix))

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE'#:psync.*=TRACE:sync.*=TRACE'
      producer.cmd(cmd)
      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      try:
          producer.cmd(cmd)
      except Exception as e:
          print ("couldn't start producer", e)
          exit(0)
      sleep(2)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumerNodes:
      cName = consumer.name
      cmd = 'export NDN_LOG=ndnsd.*=TRACE'#:psync.*=TRACE:sync.*=TRACE'
      consumer.cmd(cmd)
      cmd = 'ndnsd-consumer -s {} &> {}/{}/consumer.log -c 1 -p 1 &'.format(self.consumers[cName], self.args.workDir, cName)
      try:
          consumer.cmd(cmd)
      except Exception as e:
        print ("couldn't start producer", e)
        exit(0)
      # sleep -- let consumer boot up properly
      sleep(2)
