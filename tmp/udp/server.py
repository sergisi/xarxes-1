import socket
from ctypes import Structure, c_ubyte, c_char
HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9003   # Arbitrary non-privileged port


class Udp(Structure):
    _fields_ = [('tipus', c_ubyte ),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*50)]


if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((HOST, PORT))
    bties, addr = s.recvfrom(78) #Buffsize is 1024 bytes
    print 'Connected by', addr
    if bties:
        struc = Udp.from_buffer_copy(bties) 
        print struc.data
    respond = Udp(struc.tipus, struc.name, struc.mac, struc.random, 'Just a server response')
    s.sendto(respond, addr)
    s.close()


