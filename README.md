# GoBackN_Simulation
a GBN simulation in Clang

In this C programming assignment, you will complete the sending and receiving transport-level code for a simple reliable data transfer protocol Go-Back-N.

The program takes three input parameters from stdin as follows:
• The number of messages to simulate: the number of messages from layer 5
  that will be sent by GBN.
• Time between messages from sender's layer5: the time duration between two
  messages from layer 5. This value is usually smaller than the RTT which is set
  to 15 simulation time units.
• Channel pattern string: this string specifies the characteristics of the
  underlying channel. Each character in the string specifies the action taken by the channel for a packet being transmitted. Character ‘o’ indicates the packet will be received correctly, ‘-’ indicates the packet will be corrupted, ‘x’ indicates the packet will be lost. For example: a channel pattern string “xo-xo” indicates that 1st packet will be lost, 2nd received correctly, 3rd corrupted, 4th lost, and 5th received correctly. All the following packets will then repeat the same pattern, i.e., 6th lost, 7th received correctly and so on. Note that you can input different pattern string to test your program. Note that the sequence of the packets in channel pattern refers to both data packets from sender to receiver and the ACK packets from the receiver to sender. For example, the 1st packet is a data packet from sender to receiver, and 2nd packet may be ACK from receiver to sender (if the first data packet was not lost), 3rd packet may be a data packet again, and so on. The length of channel pattern string input by the user must be shorter than 40 characters.
• Sender’s window size: the number of packets that the sender keeps in its sending window. The default value is 8.

  
  Program Output:
  
Enter the number of messages to simulate:
5
Enter time between messages from sender's layer5 [ > 0.0]: 5
Enter channel pattern string
ooooooooooooooooooooo
Enter sender's window size
2
*********************************************************************************************************** 
Packet Transmission Log
*********************************************************************************************************** 
[5.0] A: send packet [0] base [0]
[10.0] A: send packet [1] base [0]
[11.5] B: packet [0] received, send ACK [0]
[15.0] A: buffer packet [2] base [0]
[16.5] B: packet [1] received, send ACK [1] [18.0] A: ACK [0] received
[18.0] A: send packet [2] base [1]
[20.0] A: buffer packet [3] base [1]
[23.0] A: ACK [1] received
[23.0] A: send packet [3] base [2]
[24.5] B: packet [2] received, send ACK [2] [25.0] A: buffer packet [4] base [2]
[29.5] B: packet [3] received, send ACK [3] [31.0] A: ACK [2] received
[31.0] A: send packet [4] base [3]
 
[36.0] A: ACK [3] received
[37.5] B: packet [4] received, send ACK [4] [44.0] A: ACK [4] received
Simulator terminated.
*********************************************************************************************************** 
Packet Transmission Summary
*********************************************************************************************************** 
From Sender to Receiver:
total sent pkts: 5 total correct pkts: 5 total resent pkts: 0 total lost pkts: 0 total corrupted pkts: 0
Goodput of the GBN: 29090.908 bps
