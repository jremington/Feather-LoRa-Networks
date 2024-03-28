Practical four node mesh network example. All nodes report data from each other, with occasional network connectivity reports.
Less noisy that FeatherMeshTest, only received messages and minimal network info are logged to the console. 

To run this on the SAM M0, you will need to make four numbered copies of FeatherMesh1.ino, with different values of THIS_NODE (e.g. 1 to 4).

**Example data report:** packet number (on a per node basis, to detect data loss), and node battery voltage.

**Node info report:** looks something like this:
`3 1(1,-48) 2(2,-42) 4(4,-47)`

**Interpretation:** Node 3 communicates directly with nodes 1, 2 and 4, with stated last rssi values. No hops

With hops, it might look like this: `3 1(2,-48) 2(2,-46) 4(2,-47)` (all routes from 3 start at node 2).
