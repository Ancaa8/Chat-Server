#!/usr/bin/env python

"""
The example topology creates a router and three IP subnets:

    - 192.168.0.0/24 (router-eth0, IP: 192.168.0.1)
    - 192.168.1.0/24 (router-eth1, IP: 192.168.1.1)
    - 192.168.2.0/24 (router-eth2, IP: 192.168.2.1)
"""

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Node
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.link import TCLink

class LinuxRouter(Node):
    "A Node with IP forwarding enabled."

    # pylint: disable=arguments-differ
    def config(self, **params):
        super(LinuxRouter, self).config(**params)
        # Enable forwarding on the router
        self.cmd('sysctl net.ipv4.ip_forward=1')

    def terminate(self):
        self.cmd('sysctl net.ipv4.ip_forward=0')
        super(LinuxRouter, self).terminate()

class NetworkTopo(Topo):
    "A LinuxRouter connecting three IP subnets"

    def build(self, **_opts):
        defaultIP = '192.168.0.1/24' # IP address for router-eth0
        router = self.addNode('router', cls=LinuxRouter, ip=defaultIP)

        server = self.addHost('server', ip='192.168.0.2/24',
                           defaultRoute='via 192.168.0.1')
        client1 = self.addHost('client1', ip='192.168.1.2/24',
                           defaultRoute='via 192.168.1.1')
        client2 = self.addHost('client2', ip='192.168.2.2/24',
                           defaultRoute='via 192.168.2.1')

        self.addLink(server, router, intfName1='router-eth0', params1={ 'ip' : '192.168.0.1/24' })
        self.addLink(client1, router, intfName2='router-eth1', params2={ 'ip' : '192.168.1.1/24' })
        self.addLink(client2, router, intfName2='router-eth2', params2={ 'ip' : '192.168.2.1/24' })

def run():
    topo = NetworkTopo()
    net = Mininet(topo=topo, xterms=True, link=TCLink, waitConnected=True, controller=None)
    net.start()

    CLI(net)
    net.stop()

if __name__ == '__main__':
    run()
