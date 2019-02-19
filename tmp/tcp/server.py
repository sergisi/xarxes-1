import socket
import struct
HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9002   # Arbitrary non-privileged port


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
    print struct.unpack('B7p13s7s50p', bties)
    conn.close()
    s.close()
