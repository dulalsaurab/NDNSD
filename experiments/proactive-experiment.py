import ndnsd_experiment_base as neb
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNWifiCLI, MiniNDNCLI, getPopen
from minindn.helpers.nfdc import Nfdc
from time import sleep
from minindn.minindn import Minindn

numberOfUpdates = 300
jitter = 100

if __name__ == '__main__':
    neb.subprocess.call(['rm','-r','/tmp/minindn/'])
    setLogLevel('info')
    ndn = Minindn()
    # args = ndn.args
    producers = dict()
    consumers = dict()
    producers = neb.generateNodes('P', 2, "printer", 500)
    consumers = neb.generateNodes('C', 5, "printer", 500)
    print(consumers, producers)
  
    # pass true if want to use NSLR
    exp = neb.NDNSDExperiment(ndn, producers, consumers, 'wired', True)
    # sleep(2)
    # for host in ndn.net.hosts:
    #   registerRouteToAllNeighbors(host, '/uofm/discovery/printer') 

    for host in ndn.net.hosts:
      discoveryPrefix = '/uofm/discovery/printer'
      print ("Discovery prefix: ", discoveryPrefix)
      host.cmd('nlsrc advertise {}'.format(discoveryPrefix))
      sleep(0.1)
      # set sync prefix, /discovery/printers, to multicast on all the nodes.
      Nfdc.setStrategy(host, '/uofm/discovery/printer', Nfdc.STRATEGY_MULTICAST)
    # sleep(10)

    for producer in exp.producerNodes:
        appPrefix = '/uofm/{}'.format(producer.name)
        print ("application prefix: {}".format(appPrefix))
        producer.cmd('nlsrc advertise {}'.format(appPrefix))
        sleep(0.1)
    # sleep(5)

    sleep (40)

    # lets start consumer
    for consumer in exp.consumerNodes:
      cmd = 'export NDN_LOG=ndn.Face=TRACE:ndnsd.comparision.*=TRACE'
      consumer.cmd(cmd)
      cmd = 'proactive-consumer &> {}/{}/consumer.log &'.format(ndn.args.workDir, consumer.name)
      consumer.cmd(cmd)
      sleep(1)

    # start producer
    for producer in exp.producerNodes:
      cmd = 'export NDN_LOG=ndn.Face=TRACE:ndnsd.comparision.*=TRACE'
      producer.cmd(cmd)
      cmd = 'proactive-producer /uofm/{} &> {}/{}/producer.log &'.format(producer.name, ndn.args.workDir, producer.name)
      producer.cmd(cmd)
    
    sleep(5)
    MiniNDNCLI(ndn.net)
    ndn.stop()
