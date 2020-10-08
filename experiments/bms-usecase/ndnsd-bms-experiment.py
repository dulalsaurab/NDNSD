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

def editNdnsdConf(node, f, s, v, deleteNadd=False):
  if not deleteNadd:
    node.cmd('infoedit -f {} -s {} -v {}'.format(f, s, v))
  else:
    # if deleteNadd is true, meaning, v = value is list, so each item of the list must be added to info file
    node.cmd('infoedit -f {} -d {}'.format(f, s)) 
    for i in v:
      node.cmd('infoedit -f {} -s {} -v {}'.format(f, s, i)) 

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
    self.args = args
    self.repo_obj = repo_obj    # Repo methods handler
    self.repo_group = "/discovery/repo"
    self.repos = repos
    self.sensors = sensors
    self.users = users
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
    for repo in self.repos: #repo is a dictionary
      host = self.repos[repo]['host']
      repoName = host.name
      hostInfoFile = '{}/{}/ndnsd_{}.info'.format(self.args.workDir, repoName, repoName)
      repoAppPrefix = self.repos[repo]['repoPrefix']
      servedPrefix = self.repos[repo]['servedPrefixes']
      Popen(['cp', '/usr/local/etc/ndn/ndnsd_default.info',  hostInfoFile], stdout=PIPE, stderr=PIPE).communicate()

      editNdnsdConf(host, hostInfoFile,'required.serviceName', "repo")
      editNdnsdConf(host, hostInfoFile,'required.appPrefix', repoAppPrefix)
      editNdnsdConf(host, hostInfoFile,'details.appPrefix', repoAppPrefix)
      # send the list of servedPrefix, and editNdnsdConf will handle the rest
      editNdnsdConf(host, hostInfoFile,'details.servedPrefix', servedPrefix, True)

      host.cmd('nlsrc advertise {}'.format(self.repo_group))
      host.cmd('nlsrc advertise {}'.format(repoAppPrefix))
      # host.cmd('nlsrc advertise {}/insert'.format(repoAppPrefix))
      # host.cmd('nlsrc advertise {}/delete'.format(repoAppPrefix))
      Nfdc.setStrategy(host, self.repo_group, Nfdc.STRATEGY_MULTICAST)

      cmd = 'export NDN_LOG=ndnsd.*=TRACE'
      host.cmd(cmd)
      cmd = 'ndnsd-producer {} 1 &> ndnsd.log &'.format(hostInfoFile)
      host.cmd(cmd)
      time.sleep(3)
      # start repo on the repo node
      self.repo_obj.startRepo(host, repoAppPrefix)

  def configureSensor(self):
    for sensor in self.sensors:
      host = self.sensors[sensor]['host']
      sensorName = host.name
      sensorPrefix = self.sensors[sensor]['sensorPrefix']
      serviceGroup = '/discovery/{}'.format(self.sensors[sensor]['service'])
      print("starting sensor {}".format(sensorName))

      cmd = 'export NDN_LOG=ndnsd.*=TRACE' #:psync.*=TRACE'
      host.cmd(cmd)

      host.cmd('nlsrc advertise {}'.format(sensorPrefix))
      host.cmd('nlsrc advertise {}'.format(serviceGroup))

      Nfdc.setStrategy(host, serviceGroup, Nfdc.STRATEGY_MULTICAST)
      self.sensors[sensor]["logFile"] = '{}/{}/{}.log'.format(self.args.workDir, sensorName, 'ndnsd')

      # Sensor discovers repo using ndnsd
      cmd = 'ndnsd-consumer -s {} -c 1 -p 1 &> {} &'.format("repo",  self.sensors[sensor]['logFile'])
      host.cmd(cmd)
      time.sleep(3)

  def configureUsers(self):
    for user in self.users:
      host = self.users[user]['host']
      userName = host.name
      userPrefix = self.users[user]['userPrefix']
      serviceGroup = '/discovery/{}'.format(self.users[user]['service'])
      print("Starting user node:{}".format(userPrefix))

      cmd = 'export NDN_LOG=ndnsd.*=TRACE' #:psync.*=TRACE'
      host.cmd(cmd)

      host.cmd('nlsrc advertise {}'.format(userPrefix))
      host.cmd('nlsrc advertise {}'.format(serviceGroup))

      Nfdc.setStrategy(host, serviceGroup, Nfdc.STRATEGY_MULTICAST)
      self.users[user]["logFile"] = '{}/{}/{}.log'.format(self.args.workDir, userName, 'ndnsd')

      # Consumer discovers repo and prefixes repo serves using ndnsd
      cmd = 'ndnsd-consumer -s {} -c 1 -p 1 &> {} &'.format("repo",  self.users[user]['logFile'])
      host.cmd(cmd)
      time.sleep(3)

def main():
  setLogLevel('info')
  Popen(['rm','-r', '/tmp/minindn/'], stdout=PIPE, stderr=PIPE).communicate()
  ndn = Minindn()

  repos = {
                  'r1': {'repoPrefix': '/repo/r1', 'servedPrefixes': ['/sensor/s1'], 'host': ndn.net['r1']}
               }
  sensors = {
                    's1': {'sensorPrefix': '/sensor/s1', 'host': ndn.net['s1'], 'service': 'repo'} # sensor is interested in repo service
                   }

  users = {
                'u1': {'userPrefix': '/user/u1', 'host': ndn.net['u1'], 'service': 'repo'} # user is interested in repo service, and to find what prefixes it serves
               }
  
  repo_obj = Repo()
  bms = BMS(ndn, ndn.args, repo_obj, repos, sensors, users)
  bms.configureRepo()
  bms.configureSensor()
  bms.configureUsers()

  # insert some data into the repo
  bms.users['u1']["repoNames"] = getRepoNames(bms.users['u1']['host'].name, bms.users['u1']["logFile"])
  bms.sensors['s1']["repoNames"] = getRepoNames(bms.sensors['s1']['host'].name, bms.sensors['s1']['logFile'])

  repo_obj.putDataIntoRepo(bms.sensors['s1']["host"], bms.sensors['s1']['repoNames'][0], bms.sensors['s1']["sensorPrefix"])
  time.sleep(2) #make sure data is inserted into the repo
  
  repo_obj.getDataFromRepo(bms.users['u1']["host"], bms.users['u1']["repoNames"][0], bms.sensors['s1']["sensorPrefix"])

  MiniNDNCLI(ndn.net)
  ndn.stop()

if __name__ == '__main__':
  main()
