import socket

HOST = ''     # Symbolic name meaning all available interfaces
PORT = 9002   # Arbitrary non-privileged port
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 9002))
s.listen(5)
conn, addr = s.accept()
data = conn.send("Hello C from Py\n")
print 'Connected by', addr
if data: 
    print data
data = conn.recv(150)
print data[0:5]
print hex(ord(data[7]))
conn.close()
s.close()
