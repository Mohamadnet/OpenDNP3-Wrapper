# OpenDNP3-Wrapper
This is a wrapper for OpenDNP3 1.1.0 Library written in C++ for Control-Monitoring devices based on DNP3 protocol. It opens a terminal to send and receive data to other modules of the system using JSON format. It can be easily configured with respect to the requirement of the system.

I also used internal buffer to tackle overflow because normally in network protocol there might be overflow in the peack of network. The Wrapper is tested on simulators but I haven't implemented it on any system. 

Future work:

- implementing it on Raspberry pi 4 
- Upgrading to new version


