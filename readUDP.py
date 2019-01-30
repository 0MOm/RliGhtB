# import select, socket 
from socket import *


port = 54321  # where do you expect to get a msg?
bufferSize = 1024 # whatever you need

# s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# s.bind(('<broadcast>', port))
# s.setblocking(0)

# while True:
#     result = select.select([s],[],[])
#     msg = result[0][0].recv(bufferSize) 
#     print msg


s=socket(AF_INET, SOCK_DGRAM)
s.bind(('',port))
cnt = 0
while(1):
    m=s.recvfrom(bufferSize)
    print 'len(m)='+str(len(m))
    print 'len(m[0])='+str(len(m[0]))    
    print m[0]
    
    print 'len(m[1])='+str(len(m[1]))    
    print cnt
    cnt = cnt + 1
    print m[1]      