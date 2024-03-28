Various examples of RHMesh, a self-organizing standalone mesh network with automatic route discovery, based on RadioHead RHMesh manager.

The first (FeatherMeshTest) is a rewrite of the four-node example posted at https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/ 

I strongly recommend studying that tutorial, as it offers good insights into the process of route-finding and network connectivity recovery after temporary node dropouts.

In my examples, more extensive log output was made available from all nodes so that the network response to node dropouts and reappearance can be observed from the point of view of any node.

Since the SAMD21 on the Feather M0 has no EEPROM, the node address has to be either hard coded (implemented, make four copies of the .ino file with different node addresses), or the node number could be input via analog or digital pins on each Feather separately.
