# UDPPingerServer.py

import random  # generate random lost packets
from socket import *

# Create a UDP socket, use SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)

# Assign IP address and port number to socket
serverSocket.bind(('', 12000))

while True:
    rand = random.randint(0, 10) 
    message, address = serverSocket.recvfrom(1024)  # receive client packet w/ its ip addy
    #if rand < 4:  # random lost packet
     #   continue
    #print(message)
    serverSocket.sendto(message, address)  # server responds otherwise
