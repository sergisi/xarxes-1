"""Server"""
import socket
import argparse
from ctypes import Structure, c_ubyte, c_char
import random
from time import sleep
import sys
import select
import threading


clients = {}
finished = True
threads_conf = []


REGISTER_REQ = 0x00
REGISTER_ACK = 0x01
REGISTER_NACK = 0x02
REGISTER_REJ = 0x03
ERROR = 0x09
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
W = 4  # Temps que pot tardar com a maxim send_data


class UdpPackage(Structure):
    _fields_ = [('tipus', c_ubyte),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*50)]


class TcpPackage(Structure):
    _fields_ = [('tipus', c_ubyte ),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*150)]

def string_package(package):
    return 'tipus: ' + str(package.tipus) + '\t' 'name: ' + \
           package.name + '\t' + 'MAC: ' + package.mac + '\t' + \
           'Random: ' + package.random + '\t' + 'Data: ' + package.data + '\t'

class Connexion():
    """This class will contain methods to make
       connexion releted problems easier. Once
       initialitzated, it will only be modified
       when .close() happens (at quit protocol)
       so it doesn't need a lock"""

    def __init__(self, server):
        with open(server, 'r') as servercfg:
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

    def tcp_package(self, client, tipus, data):
        return TcpPackage(tipus, self.name,
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


def failed_package_udp(tipus, data):
    return UdpPackage(tipus, '', '000000000000',
                      '000000', data)
    # TODO: ask professor if name should be '' or '000000'


def failed_package_tcp(tipus, data):
    return TcpPackage(tipus, '', '000000000000',
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
        self.is_conf = False

    def set_random(self):
        self.random = str(random.randint(0, 1000000))
        self.alives = J * GRANUL * T
        self.state = 'REGISTERED'

    def decrease_alive(self, debug):
        if self.state == 'REGISTERED' or self.state == 'ALIVE':
            self.alives -= 1
            if self.alives == 0:
                debu("Client " + self.name + " is now DISCONNECTED"
                        " due to lack of alives", debug)
                self.state = 'DISCONNECTED'

    
    def reset_alive(self):
        self.state = 'ALIVE'
        self.alives = K * GRANUL * T
    
    def __str__(self):
        none_str = lambda s: 'None' if s == None else str(s)
        return self.name + '\t' + self.mac + '\t' + none_str(self.random) \
               + '\t' + self.state + '\t'+ none_str(self.ip) + '\t' +\
               str(self.alives) + '\t' + str(self.is_conf)


def debu(line, debug):
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
    dicc = {}
    with open(equips, 'r') as equipsdat:
        for line in equipsdat:
            if line.rstrip('\n \r\t') != '':
                client = Client(line.partition(' ')[0],
                                line.partition(' ')[-1].rstrip('\n'))
                dicc[client.name] = client
        return dicc


def register(connexion, package, addr, debug):
    global clients
    if package.name in clients:
        client = clients[package.name]
        debu(client, debug)
        if client.mac != package.mac:
            connexion.udp_socket.sendto(failed_package_udp(REGISTER_NACK, 
                                                           'Failed Register '
                                                           'request: MAC '
                                                           'is not correct'),
                                                            addr)
        elif client.state == 'DISCONNECTED':
            if package.random != '000000':
                connexion.udp_socket.sendto(failed_package_udp(REGISTER_NACK, 
                                                               'Failed Register '
                                                               'request: Random is'
                                                               ' not 000000'),
                                                               addr)
            else:
                client = clients[client.name]
                client.set_random()
                client.ip = addr[0]
                print clients[client.name]
                response = connexion.udp_package(clients[package.name],
                                        REGISTER_ACK, connexion.tcp_port)
                connexion.udp_socket.sendto(response, addr)
        else: 
            if client.random != package.random:
                connexion.udp_socket.sendto(failed_package_udp(REGISTER_NACK, 
                                                          'Failed Register '
                                                          'request: Random'
                                                          ' not correct'),
                                                          addr)
            elif client.ip != addr[0]:
                connexion.udp_socket.sendto(failed_package_udp(REGISTER_NACK, 
                                                          'Failed Register '
                                                          'request: IP not'
                                                          ' correct'), addr)

            else:
                response = connexion.udp_package(clients[package.name],
                                        REGISTER_ACK, connexion.tcp_port)
                connexion.udp_socket.sendto(response, addr)
                # TODO: client.set_alives() random ask professor about it
    else:
        connexion.udp_socket.sendto(failed_package_udp(REGISTER_REJ, 
                                                    'Failed Register '
                                                    'request: client not '
                                                    'authorised'), addr)


def alive(connexion, package, addr, debug):
    global clients
    if package.name not in clients:
        connexion.udp_socket.sendto(failed_package_udp(ALIVE_REJ,
                                                    'Failed Alive '
                                                    'request: client'
                                                    ' not authorised'),
                                                    addr)
    else: 
        client = clients[package.name]
        if client.state != 'REGISTERED' and client.state != 'ALIVE':
            connexion.udp_socket.sendto(failed_package_udp(ALIVE_REJ,
                                                        'Failed Alive '
                                                        'request: client '
                                                        'not registered'),
                                                        addr)
            debu('AL_ST: ' + client.state, debug)
        elif package.mac != client.mac:
            connexion.udp_socket.sendto(failed_package_udp(ALIVE_REJ,
                                                        'Failed Alive '
                                                        'request: client MAC '
                                                        'not authorised'),
                                                        addr)
        elif addr[0] != client.ip:
            connexion.udp_socket.sendto(failed_package_udp(ALIVE_NACK,
                                                        'Failed Alive '
                                                        'request: client IP '
                                                        'not correct'),
                                                        addr)
            debu('AL_IP: ' + 'NONE' if client.ip == None else str(client.ip),
                 debug)
        elif package.random != client.random:
            connexion.udp_socket.sendto(failed_package_udp(ALIVE_NACK,
                                                        'Failed Alive request'
                                                        ': client RANDOM '
                                                        'not correct'),
                                                        addr)
        else:
            client.reset_alive()
            response = connexion.udp_package(clients[package.name],
                                    ALIVE_ACK, 'ALIVE timer reseted')
            connexion.udp_socket.sendto(response, addr)


def alive_update(debug):
    global clients
    while True:  # TODO:Change so it breaks when quit prot
        for client in clients.itervalues():
            client.decrease_alive(debug)
        sleep(1.0/GRANUL)



def udp_manager(connexion, debug):
    btties, addr = connexion.udp_socket.recvfrom(78)
    if btties:
        package = UdpPackage.from_buffer_copy(btties)
        debu(string_package(package), debug)
        if package.tipus == REGISTER_REQ:
            debu('UDP_REGISTER: ' + str(package.tipus), debug)
            register(connexion, package, addr, debug)
        elif package.tipus == ALIVE_INF:
            debu('UDP_ALIVE: ' + str(package.tipus), debug)
            alive(connexion, package, addr, debug)
        else: 
            connexion.udp_socket.sendto(failed_package_udp(ERROR,
                                                           'Failed package: '
                                                           ' not expected'
                                                           'package from client'),
                                                           addr)


def cli_manager(connexion, clients, debug):
    global finished
    line = sys.stdin.readline().rstrip('\n ')
    if line == 'list':
        list_prot(clients)
    elif line == 'quit':  # TODO: add concurrency quit protocol
        print 'Quit protocol'
        finished = False
    else:
        print 'Not a valid command. Please use list or quit'


def tcp_manager(connexion, number, debug):
    global threads_conf
    conn, addr = connexion.tcp_socket.accept()
    bties = conn.recv(178)
    package = TcpPackage.from_buffer_copy(bties)
    if package.tipus == SEND_FILE:
        debu('SEND_CONF: started', debug)
        send_conf(conn, addr, package, debug)
        debu('SEND_CONF: finished', debug)
    elif package.tipus == GET_FILE:
        debu('GET_CONF: started', debug)
        get_conf(connexion, conn, addr, package, debug)
        debu('GET_CONF: finished', debug)
    else:
        conn.send(failed_package_tcp(ERROR,
                                     'Failed package: '
                                     'not expected'
                                     'package from client'),
                                     addr)
    threads_conf[number][1] = True


def manager(connexion, debug):
    global clients
    global finished
    global threads_conf
    infiles = [connexion.udp_socket, sys.stdin, connexion.tcp_socket]
    while finished:
        inputs = select.select(infiles, [], [], 1)[0]
        for selected in inputs:
            if connexion.udp_socket == selected:
                debu('Debug: Input in udp', debug)
                thread = threading.Thread(target=udp_manager,
                                          args=(connexion, debug))
                thread.daemon = True
                thread.start()
            elif sys.stdin == selected:
                debu('Debug: Input', debug)
                thread = threading.Thread(target=cli_manager,
                                          args=(connexion, clients,
                                                debug))
                thread.daemon = True
                thread.start()
            else:  # only can be selected == connexion.tcp_socket:
                debu('Debug: Input in tcp', debug)
                thread = threading.Thread(target=tcp_manager,
                                          args=(connexion,
                                                len(threads_conf), debug))
                thread.start()
                threads_conf.append([thread, False])
        for pair in threads_conf:
            if pair[1]:
                pair[0].join()
    for pair in threads_conf:
        if pair[1]:
            pair[0].join()
    connexion.close()


def send_conf(conn, addr, first_package, debug):
    global clients
    if first_package.name not in clients:
        conn.send(failed_package_tcp(SEND_REJ,
                                     'SEND_CONF failed: '
                                     'not an authorised'
                                     'client'))
    else:
        client = clients[first_package.name]
        if client.mac != first_package.mac:
            conn.send(failed_package_tcp(SEND_REJ,
                                         'SEND_CONF failed: '
                                         'not authorised'
                                         'MAC from client'))
        elif client.random != first_package.random:
            conn.send(failed_package_tcp(SEND_NACK,
                                         'SEND_CONF failed: '
                                         'random not'
                                         'correct'))
        elif client.ip != addr[0]:
            conn.send(failed_package_tcp(SEND_NACK,
                                         'SEND_CONF failed: '
                                         'IP not'
                                         'correct'))
        elif client.is_conf:
            conn.send(failed_package_tcp(SEND_NACK,
                                         'SEND_CONF failed: '
                                         'currently under'
                                         'operation'))
        else:
            clients[client.name].is_conf = True
            response = connexion.tcp_package(client, SEND_ACK,
                                             client.name + '.cfg')
            conn.send(response)
            send_conf_protocol(conn, client, debug)
            clients[client.name].is_conf = False
    conn.close()


def send_conf_protocol(conn, client, debug):
    infiles = [conn]
    inputs = select.select(infiles, [], [], W)[0]
    with open(client.name + '.cfg', 'w') as clientcfg:
        while inputs != []:
            btties = conn.recv(178)
            if not btties:  # Means conn closed
                break
            package = TcpPackage.from_buffer_copy(btties)
            if package.tipus != SEND_END:
                clientcfg.write(package.data)
                inputs = select.select(infiles, [], [], W)[0]
            else:
                break


def get_conf(connexion, conn, addr, first_package, debug):
    global clients
    if first_package.name not in clients:
        conn.send(failed_package_tcp(GET_REJ,
                                     'GET_CONF failed: '
                                     'not an authorised'
                                     'client'))
    else:
        client = clients[first_package.name]
        if client.mac != first_package.mac:
            conn.send(failed_package_tcp(GET_REJ,
                                         'GET_CONF failed: '
                                         'not an authorised'
                                         'MAC from client'))
        elif client.random != first_package.random:
            conn.send(failed_package_tcp(GET_NACK,
                                         'GET_CONF failed: '
                                         'random is not'
                                         'correct'))
        elif client.ip != addr[0]:
            conn.send(failed_package_tcp(GET_NACK,
                                         'GET_CONF failed: '
                                         'IP is not'
                                         'correct'))
        elif clients[client.name].is_conf:
            conn.send(failed_package_tcp(GET_NACK,
                                         'GET_CONF failed: '
                                         'currently under'
                                         'operation'))
        else:
            clients[client.name].is_conf = True
            response = connexion.tcp_package(client, GET_ACK,
                                             client.name + '.cfg')
            conn.send(response)
            get_conf_protocol(connexion, conn, client, debug)
            clients[client.name].is_conf = False
    conn.close()


def get_conf_protocol(connexion, conn, client, debug):
    with open(client.name + '.cfg', 'r') as clientcfg:
        for line in clientcfg:
            response = connexion.tcp_package(client, GET_DATA, line)
            conn.send(response)
        response = connexion.tcp_package(client, GET_END, 'GET PROTOCOL ended')
        conn.send(response)


def list_prot(clients):
    print '-Nom--\t------IP------\t----MAC-----\t-ALEA-\t----ESTAT---'
    for key in clients:
        client = clients[key]
        if client.state == 'DISCONNECTED':
            random = '     -'
            ip = '             -'
        else:
            ip = str(client.ip)
            random = client.random
        print client.name + '\t' + ip + '\t' + client.mac + '\t' + \
            random + '\t' + client.state


if __name__ == '__main__':
    global clients
    args = argv()
    connexion = Connexion(args.config)
    clients = set_clients(args.authorised)
    thread = threading.Thread(target=alive_update,
                              args=(args.debug, ))
    thread.daemon = True
    thread.start()
    manager(connexion, args.debug)

