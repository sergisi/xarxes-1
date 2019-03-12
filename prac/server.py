"""Server"""
import socket
import argparse
from ctypes import Structure, c_ubyte, c_char
import random

class Udp(Structure):
    _fields_ = [('tipus', c_ubyte),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*50)]


class Connexion:
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
                self.udp_socket = self.__init_udp_socket(
                    line.partition(' ')[1])
            elif word == 'TCP-port':
                self.tcp_port = line.partition(' ')[1]
                self.tcp_socket = self.__init_tcp_socket(self.tcp_port)

    def __init_udp_socket(self, port):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind(('', port))
        return s

    def __init_tcp_socket(self, port):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('', port))
        s.listen(5)
        return s

    def close(self):
        self.tcp_socket.close()
        self.udp_socket.close()


class Client:
    """ Class for containing all client
        related data """
    def __init__(self, name, mac):
        self.name = name
        self.mac = mac
        self.random = None
        self.state = 'DISCONNECTED'
        self.ip = None

    def set_random(self):
        self.random = str(random.randint(0, 999999))



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
    pass


if __name__ == '__main__':
    args = argv()
    connexion = Connexion(args.config)