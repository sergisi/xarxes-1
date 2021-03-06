import socket
from ctypes import Structure, c_ubyte, c_char
HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9002   # Arbitrary non-privileged port


class Tcp(Structure):
    _fields_ = [('tipus', c_ubyte ),
                ('name', c_char*7),
                ('mac', c_char*13),
                ('random', c_char*7),
                ('data', c_char*150)]


if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('', 9002))
    s.listen(5)
    conn, addr = s.accept()
    bties = conn.recv(178)
    print len(bties)
    struc = Tcp.from_buffer_copy(bties)
    print struc.data
    conn.close()
    s.close()
