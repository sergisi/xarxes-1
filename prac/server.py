"""Server"""
import socket
import argparse
from ctypes import Structure, c_ubyte, c_char
import random
from time import sleep
import sys
import select

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

# Constants paquets perduts per alive
J = 2
K = 3
GRANUL = 2  # mig segon
T = 3  # Temps que triga en enviar un alive


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
        for line_with_newline in servercfg:
            line = line_with_newline.rstrip('\n')
            word = line.partition(' ')[0]
            if word == 'Nom':
                self.name = line.partition(' ')[-1]
            elif word == 'MAC':
                self.mac = line.partition(' ')[-1]
            elif word == 'UDP-port':
                self.udp_socket = init_udp_socket(
                    line.partition(' ')[-1])
            elif word == 'TCP-port':
                self.tcp_port = line.partition(' ')[-1]
                self.tcp_socket = init_tcp_socket(self.tcp_port)


    def udp_package(self, client, tipus, data):
        return UdpPackage(tipus, self.name,
                          self.mac, client.random, data)

    def close(self):
        self.tcp_socket.close()
        self.udp_socket.close()


def init_udp_socket(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', int(port)))
    return sock

def init_tcp_socket(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('', int(port)))
    sock.listen(5)
    return sock


def register_failed(tipus, data):
    return UdpPackage(tipus, '000000', '000000000000',
                      '000000', data)


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
        self.random = str(random.randint(0, 1000000))
        self.alives = J * GRANUL

    def decrease_alive(self):
        if self.state == 'REGISTERED' or self.state == 'ALIVE':
            self.alives -= 1
            if self.alives == 0:
                debug("Client " + self.name + "is now DISCONNECTED"
                        " due to lack of alives", debug)
                self.state = 'DISCONNECTED'
                self.random = None
    
    def set_alives(self):
        self.state = 'REGISTERED'
        self.alives = J * GRANUL * T
    
    def reset_alive(self):
        self.state = 'ALIVE'
        self.alives = K * GRANUL * T


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
        if line.rstrip('\n \r\t') != '':
            client = Client(line.partition(' ')[0],
                            line.partition(' ')[-1].rstrip('\n'))
            dicc[client.name] = client
    return dicc


def register(connexion, clients, package, addr, debug):
    if package.name in clients:
        client = clients[package.name]
        if client.mac != package.mac:
            connexion.udp_socket.sendto(register_failed(REGISTER_NACK, 
                                                        'Failed Registered '
                                                        'request: MAC address'
                                                        'is not correct'),
                                                         addr)
        elif client.state == 'DISCONNECTED':
            if package.random != '000000':
                connexion.udp_socket.sendto(register_failed(REGISTER_NACK, 
                                                          'Failed Registered '
                                                          'request: Random is'
                                                          ' not 000000'),
                                                          addr)
            else:
                client.set_random()
                client.ip = addr[0]
                response = connexion.udp_package(clients[package.name],
                                        REGISTER_ACK, connexion.tcp_port)
                connexion.udp_socket.sendto(response, addr)
                client.set_alives()
        else: 
            if client.random != package.random:
                connexion.udp_socket.sendto(register_failed(REGISTER_NACK, 
                                                          'Failed Registered '
                                                          'request: Random is'
                                                          ' not correct'),
                                                          addr)
            elif client.ip != addr[0]:
                connexion.udp_socket.sendto(register_failed(REGISTER_NACK, 
                                                          'Failed Registered '
                                                          'request: IP is not'
                                                          ' correct'), addr)

            else:
                response = connexion.udp_package(clients[package.name],
                                        REGISTER_ACK, connexion.tcp_port)
                connexion.udp_socket.sendto(response, addr)
                # TODO: client.set_alives() ask professor about it
    else:
        connexion.udp_socket.sendto(register_failed(REGISTER_REJ, 
                                                    'Failed Registered'
                                                    'request: client is not '
                                                    'authorised'), addr)


def alive(connexion, clients, package, addr, debug):
    if package.name not in clients:
        connexion.udp_socket.sendto(register_failed(ALIVE_REJ,
                                                    'Failed Alive '
                                                    'request: client is'
                                                    ' not authorised'),
                                                    addr)
    else: 
        client = clients[package.name]
        if client.state != 'REGISTERED' and client.state != 'ALIVE':
            connexion.udp_socket.sendto(register_failed(ALIVE_REJ,
                                                        'Failed Alive '
                                                        'request: client '
                                                        ' is not registered'),
                                                        addr)
        if package.mac != client.name:
            connexion.udp_socket.sendto(register_failed(ALIVE_REJ,
                                                        'Failed Alive '
                                                        'request: client MAC '
                                                        ' is not authorised'),
                                                        addr)
        elif addr[0] != client.ip:
            connexion.udp_socket.sendto(register_failed(ALIVE_NACK,
                                                        'Failed Alive '
                                                        'request: client IP '
                                                        ' is not correct'),
                                                        addr)
        elif package.random == client.random:
            connexion.udp_socket.sendto(register_failed(ALIVE_NACK,
                                                        'Failed Alive request'
                                                        ': client RANDOM '
                                                        ' is not correct'),
                                                        addr)
        else:
            client.reset_alive()
            response = connexion.udp_package(clients[package.name],
                                    ALIVE_ACK, 'ALIVE timer reseted')
            connexion.udp_socket.sendto(response, addr)
            client.set_alives()


def alive_update(clients):
    while True:  # TODO:Change so it breaks when quit prot
        sleep(1/GRANUL)
        for client in clients.itervalues():
            client.decrease_alive()


def udp_manager(connexion, clients, debug):
    btties, addr = connexion.udp_socket.recvfrom(78)
    if btties:
        package = UdpPackage.from_buffer_copy(btties)
        if package.tipus == REGISTER_ACK:
            register(connexion, clients, package, addr, debug)
        elif package.tipus == ALIVE_INF:
            alive(connexion, clients, package, addr, debug)
        else: 
            connexion.udp_socket.sendto(register_failed(ERROR,
                                                        'Failed package: '
                                                        ' not an expected'
                                                        'package from client'),
                                                        addr)


def cli_manager(connexion, clients, debug):
    line = sys.stdin.readline().rstrip('\n ')
    if line == 'list':
        list_prot(clients)
    elif line == 'quit':  # TODO: add concurrency quit protocol
        print 'Quit protocol'
        connexion.close()
        sys.exit(0)
    else:
        print 'Not a valid command. Please use list or quit'


def udp_server(connexion, clients, debug):
    infiles = [connexion.udp_socket, sys.stdin]
    while True:
        inputs = select.select(infiles, [], [])[0]
        if connexion.udp_socket in inputs:
            udp_manager(connexion, clients, debug)
        if sys.stdin in inputs:
            cli_manager(connexion, clients, debug) 


def list_prot(clients):
    print '-Nom--\t------IP------\t----MAC----\t-ALEA-\t----ESTAT---'
    for key in clients:
        client = clients[key]
        if client.random == None:
            random = '     -'
        if client.ip == None:
            ip = '             -'
        print client.name + '\t' + ip + '\t' + client.mac + '\t' + \
            random + '\t' + client.state


if __name__ == '__main__':
    args = argv()
    connexion = Connexion(args.config)
    clients = set_clients(args.authorised)
    udp_server(connexion, clients, args.debug)

