FeatherMesh1 is a practical four node mesh network example. All nodes report data from each other, with occasional network connectivity reports. The number of nodes is set by N_NODES, and could be as many as ten with the default RHRouter node connectivity table configuration. All data transmitted by any 3 active nodes can be harvested from the logged output of the remaining node. 

FeatherMesh1 is less verbose than FeatherMeshTest, only received messages and minimal network info are logged to the console. 

To run the 4 node version on the SAM M0, you will need to make four numbered copies of FeatherMesh1.ino, with different values of THIS_NODE (e.g. 1 to 4), and upload them to four Feather M0 LoRa modules. For two nodes only, simply reduce N_NODES to two, make two copies, and upload to two nodes.

**Example data report** `<3:0 "#799: 3959 mV"` from node, hops, packet number (on a per node basis, to detect data loss), and node battery voltage.

**Example node info report** `3 1(1,-48) 2(2,-42) 4(4,-47)`

**Interpretation:** Node 3 communicates directly with nodes 1, 2 and 4, with stated last rssi values. No hops.

With hops, it might look like this: `3 1(2,-48) 2(2,-46) 4(2,-47)` (all routes from 3 start at node 2).

**Notes on loop timing:**  For the network to respond to node dropouts and reappearances, RHMesh requires that all nodes need to attempt communication with all other nodes on a regular basis. The loop function does this, but must also switch to receive mode to listen for incoming traffic. 

When all nodes are responding, the loop time is primarily determined by the time spent listening, in the FeatherMesh1 example, a random time selected from the range (1000,3000) milliseconds, and overall network responsivity is quick. 

However, when a node drops out, several tries are made to communicate with it over a period of time (default 3, number and wait time configurable), then the message is dropped. The node is not dropped from the node list. Since the retries take considerable extra time, missing nodes drastically slow down the entire network. There does not seem to be a mechanism in RHMesh to ignore a missing node, but transmission failures are optionally logged on the console for inspection. The code to do so is in the example, but commented out.
