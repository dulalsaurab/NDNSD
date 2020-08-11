import time
import sys

from mininet.log import setLogLevel, info

from minindn.minindn import Minindn
#from minindn.util import MiniNDNCLI
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

from minindn.wifi.minindnwifi import MinindnWifi
from sys import exit
from traceback import format_exc
from minindn.util import MiniNDNWifiCLI, getPopen


numberOfUpdates = 300
jitter = 50

def cleanUp():
    pass

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
    self.producer_nodes = [host for host in ndnwifi.net.stations if host.name in self.producers]
    self.consumer_nodes = [host for host in ndnwifi.net.stations if host.name in self.consumers]
    self.start()

  def start(self):
    print("reached here")
    self.ndn.start()
    sleep(5)
    nfds = AppManager(self.ndn, self.ndn.net.stations, Nfd, logLevel='DEBUG')

    # nlsrs = AppManager(self.ndn, self.ndn.net.hosts, Nlsr)

    # info('Starting NFD on nodes\n')
    # nfds = AppManager(ndn, ndn.net.hosts, Nfd)

    for host in self.ndn.net.stations:
      host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 6363" -w {}.pcap &> /dev/null &'
               .format(host.name))
      #host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))
      time.sleep(0.005)


    # time.sleep(6)
     # copy the test.info file to system first and from there copy it to node directory
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'],
           stdout=PIPE, stderr=PIPE).communicate()

  def startProducer(self):
    print("Starting producers")
    hostInfo = dict([])
    for host in self.producer_nodes: # if host.name not in consumer:
      hostName = host.name
      hostInfoFile = self.args.workDir+'/'+hostName+'/ndnsd_'+hostName+'.info'
      appPrefix = '/ndnsd/'+hostName+'/service-info'
      
      # hostInfo[host.name] = [hostInfoFile, appPrefix, '/printer']
      # appending filename just in case if we need in future
      # self.producers[hostName].append(hostInfoFile)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile],
             stdout=PIPE, stderr=PIPE).communicate()

      host.cmd('infoedit -f {} -s required.appPrefix -v {}'.format(hostInfoFile, appPrefix))
      
      # cmd = 'nlsrc advertise {}'.format(appPrefix)
      # host.cmd(cmd)
      
      # cmd = 'nlsrc advertise {}{}'.format('/discovery/', self.producers[hostName][0])
      # host.cmd(cmd)

      Nfdc.setStrategy(host, self.producers[hostName][0], Nfdc.STRATEGY_MULTICAST)

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=TRACE'
      host.cmd(cmd)

      cmd = 'ndnsd-producer {} 1 &> {}/{}/producer.log &'.format(hostInfoFile, self.args.workDir, hostName)
      host.cmd(cmd)
      sleep(10)

  def startConsumer(self):
    print("Staring consumers")
    for consumer in self.consumer_nodes:
      cName = consumer.name

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE:psync.*=TRACE:sync.*=TRACE'
      consumer.cmd(cmd)

      # set multi-cast strategy for sync prefix
      Nfdc.setStrategy(consumer, self.consumers[cName], Nfdc.STRATEGY_MULTICAST)

      cmd = 'ndnsd-consumer -s {} &> {}/{}/consumer.log -c 1 -p 1 &'.format(self.consumers[cName],
                                                                       self.args.workDir, cName)
      consumer.cmd(cmd)
      sleep(2)
      # sleep for a while to let consumer boot up properly

if __name__ == '__main__':
    
    subprocess.call(['rm','-r','/tmp/minindn/'])


    setLogLevel('info')
    ndn = MinindnWifi()
    producers = dict()
    consumers = dict()
    producers['p1'] = ['printer', 500] #{"<sp-name>": ['service-name', 'lifetime in ms']}
    # producers['p2'] = ['printer', 900]
    # producers['p3'] = ['printer', 900]
    # consumers = {'a':'printer', 'b':'printer', 'c':'printer','d':'printer', 'e':'printer', 'f':'printer'} #['<consumer-name>']
    consumers = {'c1':'printer'} #['<consumer-name>']



    exp = NDNSDExperiment(ndn, producers, consumers)
    # sleep(2)

    # # For all host, pass ndn.net.hosts or a list, [ndn.net['a'], ..] or [ndn.net.hosts[0],.]
    # info('Adding static routes to NFD\n')
    # grh = NdnRoutingHelper(ndn.net)

    # for host in exp.producer_nodes:
    #   hostName = host.name
    #   appPrefix = '/ndnsd/'+hostName+'/service-info'
    #   discoveryPrefix = '/discovery/{}'.format(exp.producers[hostName][0])
    #   grh.addOrigin([host], [appPrefix, discoveryPrefix])

    # grh.calculateNPossibleRoutes(1)

    exp.startProducer()
    exp.startConsumer()

    # need to give some time for sync convergence
    # time.sleep(10)

    # need to run reload at producers node
    # print("Staring experiment, i.e. reloading producers, approximate time to complete: {} seconds".format(2*(numberOfUpdates + jitter)))
    # for host in exp.producer_nodes:
    #   cmd = 'ndnsd-reload -c {} -i {} -r {} &> {}/{}/reload.log &'.format(numberOfUpdates,
    #                                                                 exp.producers[host.name][1]-10,
    #                                                                 200, ndn.args.workDir,
    #                                                                 host.name)
    #   host.cmd(cmd)

    # # approximate time to complete the experiment
    # print("Sleep approximately {} seconds to complete the experiment".format(2*(numberOfUpdates + jitter)))
    # time.sleep(numberOfUpdates + jitter)
    # print("Experiment completed")

    # ndn.stop()
    # # MiniNDNCLI(ndn.net)


    # ndnwifi.start()
    # print("Starting NFD")
    # time.sleep(2)
    # nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd)

    # print("Starting pingserver...")
    # ping_server_proc = getPopen(b, "ndnpingserver /example")
    # Nfdc.createFace(a, b.IP())
    # Nfdc.registerRoute(a, "/example", b.IP())

    # print("Starting ping...")
    # NDNPingClient.ping(a, "/example", 10)

    # Start the CLI
    MiniNDNWifiCLI(ndn.net)
    ndn.net.stop()
    ndn.cleanUp()