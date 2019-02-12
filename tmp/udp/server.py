import socket

HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9003   # Arbitrary non-privileged port
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind((HOST, PORT))
data, addr = s.recvfrom(1024) #Buffsize is 1024 bytes
print 'Connected by', addr
if data: 
    print data
s.sendto("Hello from Py to C client", addr)
s.close()


