# CS 176A: Homework 3
## UDP Ping Socket Programming in C

**Authors:** Irene Evans and Mathew Kramsch

This program is used to simulate a Ping message. The client sends 10 ping messages to a server and the sever echoes back the message. The server will not send back all ten messages and will randomly choose which ones to send back. This simulates packet loss.


## Usage 
### Client Input/Output:
**Input:**  <br>
```
./PingClient IP PORT
```

Example:  <br>
```
csil-machine2> ./client_c_udp localhost 12000
```

**Output:**  <br>
```
PING received from machine_name: seq#=1 time=50 ms
PING received from machine_name: seq#=3 time=50 ms
Request Timeout
PING received from machine_name: seq#= 5 time=62 ms
--- ping statistics ---
10 packets transmitted, 8 received, 20% packet loss rtt min/avg/max = 50 62 88 ms
csil-machine2>
```

### Server Input:
```
python UDPingServer.py
```
