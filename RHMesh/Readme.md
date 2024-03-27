Various examples of RHMesh, self-organizing standalone mesh network.

The first (FeatherMeshTest) is a rewrite of the four-node example posted at https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/ 

More extensive log output is available from all nodes so that the network response to node dropouts and reappearance can be observed.
Since the SAMD21 has no EEPROM, the node address has to be either hard coded (implemented, make four copies of the .ino file with different node addresses), or node number could be input via analog or digital pins.
