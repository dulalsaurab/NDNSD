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
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper
import subprocess

import time
from random import randrange


# This function is used to edit ndnsd conf info format file
def editNdnsdConf(node, f, s, v):
  node.cmd('infoedit -f {} -s {} -v {}'.format(f, s, v))


# First we will construct repo for humidity only
class BMS(object):
  # base class, will have ndn obj, topo, start nfd, establishes routes
  def __init__(self, ndn, args, repos, sensors, users):
    self.ndn = ndn
    self.args = args
    self.repo_group = "/discovery/repo/humidity"
    self.sensor_group = "/discovery/repo/humidity/insert"
    self.user_group = "/discovery/repo/humidity/fetch"
    self.repos = [host for host in ndn.net.hosts if host.name in repos]
    self.sensors = [host for host in ndn.net.hosts if host.name in sensors]
    self.users = [host for host in ndn.net.hosts if host.name in users]
    self.start()

  def start(self):
    self.ndn.start()
    time.sleep(2)
    nfds = AppManager(self.ndn, self.ndn.net.hosts, Nfd, logLevel='DEBUG')
    nlsr = AppManager(self.ndn, self.ndn.net.hosts, Nlsr, logLevel='DEBUG')

  def ndnsd_publish():
    pass

  def configureRepo(self):

    # Configure repo for service-group: "/discovery/repo/"
    for repo in self.repos:
      repoName = repo.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, repoName, repoName)
      repoAppPrefix = '/repo/{}'.format(repoName)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile],
             stdout=PIPE, stderr=PIPE).communicate()

      editNdnsdConf(repo, hostInfoFile,'required.serviceName', "repo/humidity")
      editNdnsdConf(repo, hostInfoFile,'required.appPrefix', repoAppPrefix)
      editNdnsdConf(repo, hostInfoFile,'optional.description', "Publishing under {}".format(repoAppPrefix))

      # set sync and info prefix to multi-cast strategy
      # Nfdc.registerRoute(repo, "/discovery/repo/humidity", '224.0.23.170')
      repo.cmd('nlsrc advertise {}'.format(self.repo_group))
      repo.cmd('nlsrc advertise {}'.format(repoAppPrefix))
      Nfdc.setStrategy(repo, self.repo_group, Nfdc.STRATEGY_MULTICAST)

      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE'#:psync.*=TRACE:sync.*=TRACE'
      repo.cmd(cmd)

      cmd = 'ndnsd-producer {} 1 &> {}/{}/repo_group.log &'.format(hostInfoFile,
                                                                 self.args.workDir,
                                                                 repoName)
      repo.cmd(cmd)
      time.sleep(2)

    # Configure repo for service-group: "/discovery/repo/humidity/insert"
    # this updates is for sensors
    for repo in self.repos:
      repoName = repo.name
      hostInfoFile = '{}/{}/ndnsd_{}_{}.info'.format(self.args.workDir, repoName, repoName, 'insert')
      repoAppPrefix = '/repo/{}/{}'.format(repoName, 'insert')
      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info', hostInfoFile],
             stdout=PIPE, stderr=PIPE).communicate()

      editNdnsdConf(repo, hostInfoFile,'required.serviceName', "repo/humidity/insert")
      editNdnsdConf(repo, hostInfoFile,'required.appPrefix', repoAppPrefix)
      editNdnsdConf(repo, hostInfoFile,'optional.description', "Accepting under {}".format(repoAppPrefix))

      # set sync and info prefix to multi-cast strategy
      # Nfdc.registerRoute(repo, "/discovery/repo/humidity", '224.0.23.170')
      repo.cmd('nlsrc advertise {}'.format(self.sensor_group))
      repo.cmd('nlsrc advertise {}'.format(repoAppPrefix))
      Nfdc.setStrategy(repo, self.sensor_group, Nfdc.STRATEGY_MULTICAST)

      cmd = 'ndnsd-producer {} 1 &> {}/{}/repo_for_sensor.log &'.format(hostInfoFile,
                                                                 self.args.workDir,
                                                                 repoName)
      repo.cmd(cmd)
      time.sleep(2)


  def configureSensor(self):
    for sensor in self.sensors:
      sensorName = sensor.name
      # uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE' #:psync.*=TRACE:sync.*=TRACE'
      sensor.cmd(cmd)

      # set multi-cast strategy for sync prefix
      Nfdc.setStrategy(sensor, '/discovery/repo/humidity/insert', Nfdc.STRATEGY_MULTICAST)

      cmd = 'ndnsd-consumer -s {} &> {}/{}/sensor.log -c 1 -p 1 &'.format('repo/humidity/insert',
                                                                          self.args.workDir, sensor)
      sensor.cmd(cmd)

      # sleep for a while to let consumer boot up properly
      time.sleep(2)


  def startUser():
    pass


def main():
  setLogLevel('info')
  subprocess.call(['rm','-r','/tmp/minindn/'])
  
  repos = ['r1', 'r2']
  sensors = ['s1']
  users = ['u1']
  gateways = ['g1', 'g2']

  ndn = Minindn()

  bms = BMS(ndn, ndn.args, repos, sensors, users)
  bms.configureRepo()
  bms.configureSensor()

 
  MiniNDNCLI(ndn.net)
  ndn.stop()

if __name__ == '__main__':
  main()



