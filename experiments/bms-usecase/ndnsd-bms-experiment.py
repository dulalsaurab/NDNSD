import time
from subprocess import Popen, PIPE
import argparse

from mininet.log import setLogLevel, info
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc

from minindn.apps.nlsr import Nlsr
from repo import Repo

def editNdnsdConf(node, f, s, v):
  node.cmd('infoedit -f {} -s {} -v {}'.format(f, s, v))

def getRepoNames(node, logFile):
  # Parse logs and get names of repos (temporary)
  repoNames = []
  with open(logFile, 'r') as f:
    for line in f.readlines():
      if "appPrefix" in line:
        repoNames.append(line.split(":")[-1].strip())
  return repoNames

class BMS(object):
  def __init__(self, ndn, args, repo_obj, repos, sensors, users):
    self.ndn = ndn
    self.repo_obj = repo_obj
    self.args = args
    self.repo_group = "/discovery/repo"
    self.repos = [host for host in ndn.net.hosts if host.name in repos]
    self.sensors = {'hosts': [host for host in ndn.net.hosts if host.name in sensors] }
    self.users = {'hosts': [host for host in ndn.net.hosts if host.name in users]}
    self.start()

  def start(self):
    self.ndn.start()
    AppManager(self.ndn, self.ndn.net.hosts, Nfd, logLevel='INFO')
    AppManager(self.ndn, self.ndn.net.hosts, Nlsr, logLevel='INFO')
    Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate()
    # give some time for route computation and installation
    time.sleep(20)

  def configureRepo(self):
    # Configure repo for service-group: "/discovery/repo/"
    for repo in self.repos:
      repoName = repo.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, repoName, repoName)
      repoAppPrefix = '/repo/{}'.format(repoName)

      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info',  hostInfoFile], stdout=PIPE, stderr=PIPE).communicate()

      editNdnsdConf(repo, hostInfoFile,'required.serviceName', "repo")
      editNdnsdConf(repo, hostInfoFile,'required.appPrefix', repoAppPrefix)
      editNdnsdConf(repo, hostInfoFile,'details.appPrefix', repoAppPrefix)
      editNdnsdConf(repo, hostInfoFile,'details.servedPrefix', "/sensor/s1")

      repo.cmd('nlsrc advertise {}'.format(self.repo_group))
      repo.cmd('nlsrc advertise {}'.format(repoAppPrefix))
      repo.cmd('nlsrc advertise {}/insert'.format(repoAppPrefix))
      repo.cmd('nlsrc advertise {}/delete'.format(repoAppPrefix))
      Nfdc.setStrategy(repo, self.repo_group, Nfdc.STRATEGY_MULTICAST)

      # Uncomment to enable sync log
      cmd = 'export NDN_LOG=ndnsd.*=TRACE'
      repo.cmd(cmd)
      cmd = 'ndnsd-producer {} 1 &> ndnsd.log &'.format(hostInfoFile)
      repo.cmd(cmd)
      time.sleep(3)
      # start repo on the repo node
      self.repo_obj.startRepo(repo, repoAppPrefix)

  def configureSensor(self):
    for sensor in self.sensors["hosts"]:
      sensorName = sensor.name
      sensorPrefix =  '/sensor/{}'.format(sensorName)
      print("starting sensor {}".format(sensorName))
      cmd = 'export NDN_LOG=ndnsd.*=TRACE' #:psync.*=TRACE'
      sensor.cmd(cmd)
      sensor.cmd('nlsrc advertise {}'.format(sensorPrefix))
      sensor.cmd('nlsrc advertise {}'.format(self.repo_group))

      Nfdc.setStrategy(sensor, '/discovery/repo', Nfdc.STRATEGY_MULTICAST)
      self.sensors["logFile"] = '{}/{}/{}.log'.format(self.args.workDir, sensorName, 'ndnsd')
      cmd = 'ndnsd-consumer -s {} -c 1 -p 1 &> {} &'.format("repo",  self.sensors['logFile'])
      sensor.cmd(cmd)
      time.sleep(3)

  def configureUsers(self):
    for user in self.users["hosts"]:
      userName = user.name
      userPrefix = '/user/{}'.format(userName)
      print("Starting user node:{}".format(userPrefix))
      cmd = 'export NDN_LOG=ndnsd.*=TRACE' #:psync.*=TRACE'
      user.cmd(cmd)
      user.cmd('nlsrc advertise {}'.format(userPrefix))
      user.cmd('nlsrc advertise {}'.format(self.repo_group))
      Nfdc.setStrategy(user, '/discovery/repo', Nfdc.STRATEGY_MULTICAST)
      self.users["logFile"] = '{}/{}/{}.log'.format(self.args.workDir, userName, 'ndnsd')
      cmd = 'ndnsd-consumer -s {} -c 1 -p 1 &> {} &'.format("repo",  self.users['logFile'])
      user.cmd(cmd)
      time.sleep(3)

def main():
  setLogLevel('info')
  Popen(['rm','-r', '/tmp/minindn/'], stdout=PIPE, stderr=PIPE).communicate()
  repos = ['r1']
  sensors = ['s1']
  users = ['u1']
  gateways = ['g1']

  ndn = Minindn()
  repo_obj = Repo()
  bms = BMS(ndn, ndn.args, repo_obj, repos, sensors, users)
  bms.configureRepo()
  bms.configureSensor()
  bms.configureUsers()

  # insert some data into the repo
  bms.users["repoNames"] = getRepoNames(bms.users['hosts'][0].name, bms.users["logFile"])
  bms.sensors["repoNames"] = getRepoNames(bms.sensors['hosts'][0].name, bms.sensors['logFile'])

  repo_obj.putDataIntoRepo(bms.sensors["hosts"][0], bms.sensors['repoNames'][0], '/sensor/s1')
  time.sleep(2) #make sure data is inserted into the repo
  
  repo_obj.getDataFromRepo(bms.users["hosts"][0], bms.users["repoNames"][0], "/sensor/s1")

  MiniNDNCLI(ndn.net)
  ndn.stop()

if __name__ == '__main__':
  main()
