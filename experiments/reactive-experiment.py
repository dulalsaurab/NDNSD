import ndnsd_experiment_base as neb
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNWifiCLI, MiniNDNCLI, getPopen
from minindn.helpers.nfdc import Nfdc
from time import sleep
from minindn.minindn import Minindn
from nlsr_common import getParser

numberOfUpdates = 2
jitter = 100

if __name__ == '__main__':
    neb.subprocess.call(['rm','-r','/tmp/minindn/'])
    setLogLevel('info')
    ndn = Minindn(parser=getParser())
    # args = ndn.args
    producers = dict()
    consumers = dict()

    consumers = {'wu': 'printer', 'arizona': 'printer', 'uiuc': 'printer', 'uaslp': 'printer', 'csu': 'printer',
                'neu': 'printer', 'basel': 'printer', 'copelabs': 'printer', 'uclacs': 'printer', 'bern': 'printer'}
    producers = {'ucla': ['printer', 10000], 'memphis': ['printer', 10000]}

    # producers = neb.generateNodes('P', 2, "printer", 500)
    # consumers = neb.generateNodes('C', 5, "printer", 500)
    print(consumers, producers)
    
  
    # pass true if want to use NSLR
    exp = neb.NDNSDExperiment(ndn, producers, consumers, 'wired', True)
    sleep(2)
    discoveryPrefix = '/uofm/discovery/printer'

    # for producer in exp.producerNodes:
    #     appPrefix = '/uofm/{}'.format(producer.name)
    #     producer.cmd('nlsrc advertise {}'.format(appPrefix))
    #     sleep(5)
    #     # producer.cmd('nlsrc advertise {}'.format(discoveryPrefix))

    for producer in exp.producerNodes:
      producer.cmd('nlsrc advertise {}'.format(discoveryPrefix))
      sleep(5)
    
    # set discovery prefix, /uofm/discovery/printers, to multicast on all the nodes.
    for host in ndn.net.hosts:
      Nfdc.setStrategy(host, '/uofm/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      sleep(2)

    # sleep (60)
    # lets start consumer
    for consumer in exp.consumerNodes:
      cmd = 'export NDN_LOG=ndn.Face=TRACE:ndnsd.comparision.*=TRACE'
      consumer.cmd(cmd)
      cmd = 'reactive-consumer &> {}/{}/consumer.log &'.format(ndn.args.workDir, consumer.name)
      consumer.cmd(cmd)
      sleep(1)

    # start producer
    for producer in exp.producerNodes:
      cmd = 'export NDN_LOG=ndn.Face=TRACE:ndnsd.comparision.*=TRACE'
      producer.cmd(cmd)
      cmd = 'reactive-producer /uofm/{} &> {}/{}/producer.log &'.format(producer.name, ndn.args.workDir, producer.name)
      producer.cmd(cmd)

    sleep(320)
    # MiniNDNCLI(ndn.net)
    ndn.stop()
