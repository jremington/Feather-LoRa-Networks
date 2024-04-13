Text networking with Feather M0!

Serial input on port Serial1 with the format node:message sends up to 80 characters of "message" to node. Node number must be in range 1:N_NODES.
Since this are generally more important than data messages, many tries are made to send the message (15 as currently programmed).

It is intended that Serial1 be connected to a Bluetooth module like HC-06, to facilitate sending text messages via mobile phone.

All nodes report ASCII data messages addressed to them by data nodes, such as those programmed with the code in the RHMesh subfolder.
