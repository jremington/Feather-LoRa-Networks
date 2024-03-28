Practical four node mesh network example. All nodes report data from each other, with occasional network connectivity reports.
Less noisy that FeatherMeshTest, only received messages and minimal network info are logged to the console. 

To run this on the SAM M0, you will need to make four numbered copies of FeatherMesh1.ino, with different values of THIS_NODE (e.g. 1 to 4).

**Example data report** `<3:0 "#799: 3959 mV"` from node, hops, packet number (on a per node basis, to detect data loss), and node battery voltage.

**Node info report** looks something like this:
`3 1(1,-48) 2(2,-42) 4(4,-47)`

**Interpretation:** Node 3 communicates directly with nodes 1, 2 and 4, with stated last rssi values. No hops

With hops, it might look like this: `3 1(2,-48) 2(2,-46) 4(2,-47)` (all routes from 3 start at node 2).

**NOTES on loop timing:**  For the network to respond to node dropouts and reappearances, RHMesh requires that all nodes need to attempt communication with all other nodes on a regular basis. The loop function does this, but must also switch to recieve mode to listen for incoming traffic. 

When all nodes are responding, the loop time is primarily determined by the time spent listening, in the FeatherMesh1 example, a random time selected from the range (1000,3000) milliseconds, and overall network responsivity is quick. 

However, when a node drops out, several tries (default 3, configurable) are made to communicate with it over a period of time, then the message is dropped. The node is not dropped from the node list. Since the retries take considerable extra time, missing nodes slows down the entire network. There does not seem to be a mechanism in RHMesh to ignore a node, but transmission failures are optionally logged on the console. The code to do so is in the example, but commented out.
