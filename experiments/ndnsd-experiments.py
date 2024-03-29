import ndnsd_experiment_base as neb
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNWifiCLI, MiniNDNCLI, getPopen
from minindn.helpers.nfdc import Nfdc
from time import sleep
from minindn.minindn import Minindn
from nlsr_common import getParser

numberOfUpdates = 30
jitter = 60

if __name__ == '__main__':
    neb.subprocess.call(['rm','-r','/tmp/minindn/'])
    setLogLevel('info')
    ndn = Minindn(parser=getParser())
    
    producers = dict()
    consumers = dict()

    consumers = {'wu': 'printer', 'arizona': 'printer', 'uiuc': 'printer', 'uaslp': 'printer', 'csu': 'printer',
                'neu': 'printer', 'basel': 'printer', 'copelabs': 'printer', 'uclacs': 'printer', 'bern': 'printer'}
    producers = {'ucla': ['printer', 10000], 'memphis': ['printer', 10000]}

    # can call generate nodes if consumers are c1, c2.... and producers are p1, p2 ....
    # producers = neb.generateNodes('P', 2, "printer", 1000)
    # consumers = neb.generateNodes('C', 5, "printer", 1000)
    print(consumers, producers)

    # pass true if want to use NSLR
    exp = neb.NDNSDExperiment(ndn, producers, consumers, 'wired', True)
    sleep(2)

    for producer in exp.producerNodes:
        hostName = producer.name
        appPrefix = '/ndnsd/{}/service-info'.format(hostName)
        discoveryPrefix = '/discovery/{}'.format(exp.producers[hostName][0])
        print ("Producer: {} - discovery prefix: {}".format(hostName, discoveryPrefix))
        producer.cmd('nlsrc advertise {}'.format(appPrefix))
        producer.cmd('nlsrc advertise {}'.format(discoveryPrefix))
        sleep(5)

    for consumer in exp.consumerNodes:
        hostName = consumer.name
        discoveryPrefix = '/discovery/{}'.format(exp.consumers[hostName])
        print ("Consumer: {} - discovery prefix: {}".format(hostName, discoveryPrefix))
        consumer.cmd('nlsrc advertise {}'.format(discoveryPrefix))
        sleep(5)

     # set sync prefix, /discovery/printers, to multicast on all the nodes.
    for host in ndn.net.hosts:
      Nfdc.setStrategy(host, '/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      sleep(2)

    exp.startProducer()
    exp.startConsumer()

    # need to run reload at producers node
    print("Staring experiment, i.e. reloading producers, approximate time to complete: {} seconds".format(numberOfUpdates + jitter))
    for host in exp.producerNodes:
      appPrefix = '/ndnsd/{}/service-info'.format(host.name)
      cmd = 'ndnsd-reload -c {} -i {} -r {} -p {} &> {}/{}/reload.log &'.format(numberOfUpdates, exp.producers[host.name][1],
                                                                                                                            100, appPrefix, ndn.args.workDir, host.name)
      host.cmd(cmd)

    # uncomment the line below to enable CLI
    # MiniNDNCLI(ndn.net)

    # adjust approximate time to complete the experiment based on numberOfUpdates
    print("Sleep approximately {} seconds to complete the experiment".format(10*numberOfUpdates + jitter))
    sleep(10*numberOfUpdates + jitter)
    print("Experiment completed")
    ndn.stop()
