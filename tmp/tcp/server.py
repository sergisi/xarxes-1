import socket
from ctypes import *
HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9002   # Arbitrary non-privileged port


class Pac1(Structure):
    _fields_ = [('tipus', c_ubyte ),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*50)]


if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('', 9002))
    s.listen(5)
    conn, addr = s.accept()
    data = conn.send("Hello C from Py\n")
    print 'Connected by', addr
    if data: 
        print data
    bties = conn.recv(78)
    print len(bties)
    struc = Pac1.from_buffer_copy(bties)
    print struc.type
    conn.close()
    s.close()
