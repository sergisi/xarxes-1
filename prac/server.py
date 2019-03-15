"""Server"""
import socket
import argparse
from ctypes import Structure, c_ubyte, c_char
import random
from time import sleep

REGISTER_REQ = 0x00
REGISTER_ACK = 0x01
REGISTER_NACK = 0x02
REGISTER_REJ = 0x03
ERROR = 0x09,

ALIVE_INF = 0x10
ALIVE_ACK = 0x11
ALIVE_NACK = 0x12
ALIVE_REJ = 0x13

SEND_FILE = 0x20
SEND_ACK = 0x21
SEND_NACK = 0x22
SEND_REJ = 0x23
SEND_DATA = 0x24
SEND_END = 0x25

GET_FILE = 0x30
GET_ACK = 0x31
GET_NACK = 0x32
GET_REJ = 0x33
GET_DATA = 0x34
GET_END = 0x35

# Constants temps
J = 2
K = 3
GRANUL = 2  # mig segon


class UdpPackage(Structure):
    _fields_ = [('tipus', c_ubyte),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*50)]


class Connexion():
    """This class will contain methods to make
       connexion releted problems easier."""

    def __init__(self, server):
        servercfg = open(server)
        for line in servercfg:
            word = line.partition(' ')[0]
            if word == 'Nom':
                self.name = line.partition(' ')[1]
            elif word == 'MAC':
                self.mac = line.partition(' ')[1]
            elif word == 'UDP-port':
                self.udp_socket = __init_udp_socket(
                    line.partition(' ')[1])
            elif word == 'TCP-port':
                self.tcp_port = line.partition(' ')[1]
                self.tcp_socket = __init_tcp_socket(self.tcp_port)


    def udp_package(self, client, tipus, data):
        return UdpPackage(tipus, self.name,
                          self.mac, client.random, data)

    def close(self):
        self.tcp_socket.close()
        self.udp_socket.close()


def __init_udp_socket(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', port))
    return sock

def __init_tcp_socket(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', port))
    sock.listen(5)
    return sock

class Client:
    """ Class for containing all client
        related data """
    def __init__(self, name, mac):
        self.name = name
        self.mac = mac
        self.random = None
        self.state = 'DISCONNECTED'
        self.ip = None
        self.alives = 0

    def set_random(self):
        self.random = str(random.randint(0, 999999))
        self.alives = J * GRANUL

    def decrease_alive(self):
        if self.state == 'REGISTERED' or self.state == 'ALIVE':
            self.alives -= 1
            if self.alives == 0:
                debug("Client " + self.name + "is now DISCONNECTED"
                        " due to lack of alives", debug)
                self.state = 'DISCONNECTED'
    
    def reset_alive(self):
        self.alives = K * GRANUL


def debug(line, debug):
    if debug:
        print line


def argv():
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--debug", action="store_true",
                        default=False, help="activates debug mode")
    parser.add_argument("-c", "--config", type=str, default="server.cfg",
                        help="adds a configuration file to the server")
    parser.add_argument('-u', '--authorised', type=str, default="equips.dat",
                        help='adds a authorised clients file to the server')
    return parser.parse_args()


def set_clients(equips):
    equipsdat = open(equips)
    dicc = {}
    for line in equipsdat:
        client = Client(line.partition(' ')[0],
                        line.partition(' ')[1])
        dicc[client.name] = client
    return dicc


def register(connexion, clients, package, addr, debug):
    if reg(package) == 0:  # It will be deleated with proper elif statements
        actual = clients[package.name]
        actual.set_random()
        actual.ip = addr
        response = connexion.udp_package(clients[package.name],
                               REGISTER_ACK, str(connexion.tcp_port))
        connexion.udp_socket.sendto(response, addr)


def reg(package):
    return 0


def alive(connexion, clients, package, addr, debug):
    pass


# TODO: finish this
def alive_update(clients):
    while True:  # Change so it breaks when quit prot
        sleep(1/GRANUL)
        for client in clients.itervalues():
            client.decrease_alive()


def udp_server(connexion, clients, debug):
    btties, addr = connexion.udp_socket.recvfrom(78)
    if btties:
        package = UdpPackage.from_buffer_copy(btties)
        if package.name in clients:
            if package.tipus < 0x10:
                register(connexion, clients, package, addr, debug)
            else:
                alive(connexion, clients, package, addr, debug)


# TODO: revise list protocol
def list_prot(clients):
    print '-Nom--\t------IP------\t----MAC----\t-ALEA-\t----ESTAT---'
    for client in clients:
        print '%s\t%s\t%s\t%s\t%s', (client.name, str(client.ip[0]),
                                     client.mac, client.random,
                                     client.state)


if __name__ == '__main__':
    args = argv()
    connexion = Connexion(args.config)
    clients = set_clients(args.authorised)
