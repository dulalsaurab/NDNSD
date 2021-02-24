import ndnsd_experiment_base as neb
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNWifiCLI, getPopen
from minindn.helpers.nfdc import Nfdc
from time import sleep 
from minindn.minindn import Minindn

numberOfUpdates = 10
jitter = 5

if __name__ == '__main__':
    neb.subprocess.call(['rm','-r','/tmp/minindn/'])
    setLogLevel('info')
    ndn = Minindn()
    producers = dict()
    consumers = dict()
    producers = neb.generateNodes('P', 1)
    consumers = neb.generateNodes('C', 1)
    print(consumers, producers)
    # pass true if want to use NSLR
    exp = neb.NDNSDExperiment(ndn, producers, consumers, 'wired', True)
    sleep(2)

    # set sync prefix, /discovery/printers, to multicast on all the nodes.
    for host in ndn.net.hosts:
      Nfdc.setStrategy(host, '/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      sleep(2)

    for producer in exp.producerNodes:
        hostName = producer.name
        appPrefix = '/ndnsd/{}/service-info'.format(hostName)
        discoveryPrefix = '/discovery/{}'.format(exp.producers[hostName][0])
        host.cmd('nlsrc advertise {}'.format(appPrefix))
        host.cmd('nlsrc advertise {}'.format(discoveryPrefix))
        sleep(0.1)
   
    exp.startProducer()
    exp.startConsumer()

    # need to run reload at producers node
    print("Staring experiment, i.e. reloading producers, approximate time to complete: {} seconds".format(numberOfUpdates + jitter))
    for host in exp.producerNodes:
      appPrefix = '/ndnsd/{}/service-info'.format(host.name)
      cmd = 'ndnsd-reload -c {} -i {} -r {} -p {} &> {}/{}/reload.log &'.format(numberOfUpdates, exp.producers[host.name][1]-10,
                                                                                                                            100, appPrefix, ndn.args.workDir, host.name)
      host.cmd(cmd)

    # approximate time to complete the experiment
    print("Sleep approximately {} seconds to complete the experiment".format(2*numberOfUpdates + jitter))
    sleep(numberOfUpdates + jitter)
    print("Experiment completed")
    ndn.stop()