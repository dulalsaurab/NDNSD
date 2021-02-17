#TODO: convert this experiment to wireless experiment 

from mininet.log import setLogLevel, info
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.apps.nlsr import Nlsr
import time
import subprocess
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen

def setMTUsize(ndn, mtu):
    for host in ndn.net.hosts:
        for intf in host.intfList():
            host.cmd("ifconfig {} mtu {}".format(intf, mtu))

def setTsharkLog (net):
    for host in net.hosts:
        print ("came here:", host.name)
        # host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "port 6363" -w /tmp/{}.tshark &'.format(host.name))
        print(host.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 5353" -w {}.pcap &> /dev/null &'.format(host.name)))
        # host.cmd('sudo tcpdump port 5353 2>&1 > abc.txt &')   
    time.sleep(5)
        # host.cmd('ndndump -i any &> {}.ndndump &'.format(host.name))

def start(ndn):
    ndn.start()
    time.sleep(5)
    setMTUsize(ndn, 9000)
    nfds = AppManager(ndn, ndn.net.hosts, Nfd, logLevel='DEBUG')
    time.sleep(0.1)
    # info('Starting NLSR on nodes\n')
    # nlsrs = AppManager(ndn, ndn.net.hosts, Nlsr)
    # time.sleep(50)
    # Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate()
    # print ("did i came here")
    # setTsharkLog(ndn.net)

def startProducer (producer):
    # use avahi and publish service
    producer.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 5353" -w {}.pcap &> /dev/null &'.format(producer.name))
    cmd = 'echo "Producer publish" | ts "%Y%m%d-%H:%M:%.S" &> avahi.log &'
    producer.cmd(cmd)
    cmd = 'avahi-publish-service {} {} {} {} | ts  "%Y%m%d-%H:%M:%.S" &>> avahi.log &'.format('printer', '_ndnsd._udp', 5683, '/mylight-myhouse')
    producer.cmd(cmd)
    time.sleep(0.1)
    # Popen(['cp', 'test.info', '/usr/local/etc/ndn/ndnsd_default.info'], stdout=PIPE, stderr=PIPE).communicate() 

def startConsumer (consumers):
    for consumer in consumers:
        consumer.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 5353" -w {}.pcap &> /dev/null &'.format(consumer.name))
        cmd = 'echo "fetching service" | ts "%Y%m%d-%H:%M:%.S" &> avahi.log &'
        consumer.cmd (cmd) 
        cmd = 'avahi-browse -rt {} | ts "%Y%m%d-%H:%M:%.S" &>> avahi.log &'.format('_ndnsd._udp')
        consumer.cmd (cmd)

if __name__ == '__main__':
    subprocess.call(['rm','-rf','/tmp/minindn/*'])
    subprocess.call(['killall','tshark'])
    setLogLevel('info')
    ndnwifi = MinindnWifi()
    producers = ndnwifi.net['p1']
    consumers = [ndnwifi.net['c1']]
    # consumers = [ndn.net['b'], ndn.net['c'], ndn.net['d']] 
    start(ndnwifi)

    startProducer(producers)
    startConsumer(consumers)

    MiniNDNWifiCLI(ndnwifi.net)
    subprocess.call(['killall','tshark'])
    ndnwifi.stop()
    ndnwifi.cleanUp()
