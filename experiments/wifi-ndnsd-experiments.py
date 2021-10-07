import ndnsd_experiment_base as neb
from mininet.log import setLogLevel, info
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen
from minindn.helpers.nfdc import Nfdc
from time import sleep

numberOfUpdates = 10
jitter = 5

if __name__ == '__main__':
    neb.subprocess.call(['rm','-r','/tmp/minindn/'])
    setLogLevel('info')
    ndn = MinindnWifi()
    producers = dict()
    consumers = dict()
    producers = neb.generateNodes('P', 2)
    consumers = neb.generateNodes('C', 3)
    print(consumers, producers)
    exp = neb.NDNSDExperiment(ndn, producers, consumers, 'wifi')
    sleep(2)
    exp.startProducer()
    exp.startConsumer()

    # neb.registerRouteToAllNeighbors(ndn, hosts, '/discovery/printer')
    # set sync prefix, /discovery/printers, to multicast on all the stations.
    for node in ndn.net.stations:
      Nfdc.registerRoute(node, '/discovery/printer', '224.0.23.170')
      Nfdc.registerRoute(node, '/ndnsd', '224.0.23.170')
      Nfdc.setStrategy(node, '/discovery/printer', Nfdc.STRATEGY_MULTICAST)
      sleep(1)

    # sleep -- time for sync convergence
    sleep(10)

    # need to run reload at producers node
    print("Staring experiment, i.e. reloading producers, approximate time to complete: {} seconds".format(numberOfUpdates))
    for host in exp.producerNodes:
        cmd = 'ndnsd-reload -c {} -i {} -r {} &> {}/{}/reload.log &'.format(numberOfUpdates, exp.producers[host.name][1]-10, 100, 
                                                                                                                      ndn.args.workDir, host.name)
        host.cmd(cmd)

    # approximate time to complete the experiment
    print("Sleep approximately {} seconds to complete the experiment".format((numberOfUpdates + jitter)))
    sleep(numberOfUpdates)
    print("Experiment completed")

    # MiniNDNWifiCLI(ndn.net)
    ndn.net.stop()
    ndn.cleanUp()
