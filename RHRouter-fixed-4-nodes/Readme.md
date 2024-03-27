These examples are minimally edited from the original RadioHead RF22 RHRouter examples for a fixed-node network with one client and three servers. 
Network connectivity, as described by the routing tables, is assumed to be 1-2-3-4. That is, only nearest neighbors can communicate with each other,
so a message from 1 to 4 requires 2 hops via repeater nodes 2 and 3. 

The network was tested by uncommenting `#define RH_NETWORK_TEST 1` in RHRouter.h, which enforces this rule regardless of actual connectivity.

In this example, the client originates a message to the end server (SERVER3), which responds to the originator to acknowledge receipt. 
Message delivery is reliable and automatically acknowledged by intermediate nodes.
