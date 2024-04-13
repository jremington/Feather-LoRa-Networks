Text networking with Feather M0!

ASCII text messages via Serial1 input with the format N:message are accepted. Up to 80 characters of "message" will be transmitted to node N. Node number N must be in range 1:N_NODES. 

Messages to self are not supported and are ignored. The mesh network will route messages through visible nodes if the target node is not directly visible to the sender.

Since these message would normally be regarded as more important than data messages, many tries are made to send the message (15 as currently programmed).

It is intended that Serial1 be connected to a Bluetooth module like HC-06, to facilitate sending text messages via mobile phone running a bluetooth terminal app.

All nodes report ASCII data messages addressed to them by data nodes, such as those nodes programmed with the code in the RHMesh subfolder.
