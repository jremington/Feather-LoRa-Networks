This example demonstrates one approach to making a very simple two-way repeater with three nodes (two clients and one repeater), based on RH_ReliableDatagram.  Each client can transmit an acknowledged, error-free data message to the other client, via a line-of-sight repeater. Delivery is acknowledged on a hop-to-hop basis, but there is no provision in RHMesh for end-to-end acknowledgements.

Transmission of text messages via the Serial1 port is also possible with this setup, see the examples in directory 2node-text directory for Serial1 message input code.
