# Feather-LoRa-Networks

This repository contains several working, tested examples of standalone LoRa networks based on the RadioHead library. They were developed and tested on the Adafruit Feather M0 LoRa module, but should work with any Feather radio that is supported by the RadioHead library.  

Examples include a self-organizing, standalone mesh network, based on the RHMesh manager, and fixed node repeater networks based on the RHRouter and RHReliableDatagram managers.

Message traffic features acknowledged, optionally AES128-encrypted, reliable message delivery to intermediate nodes, but in multi-hop networks, the ultimate target does not automatically acknowledge to the originator of successful delivery. In other words, end-to-end message receipt is not acknowledged. Another network layer or mechanism is needed for reliable end-to-end message delivery or, for example, file transfer.  Node to node text messages, entered via a Bluetooth connection to a smart phone or laptop are supported.

In countless hours of tests, I have not encountered any obvious bugs in the RadioHead library. It is a solid, very reliable and very professional contribution to the community.

Some other posted and recommended open source examples of Feather mesh networks:
```
https://github.com/royyandzakiy/LoRa-RHMesh
https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/
```

# Example of FeatherBT_OLED node in 3D printed case

Each node like that pictured below contains a Feather radio with OLED display, battery, switch and an HC-06 UART serial Bluetooth module, which can accept text messages from a paired mobile phone or computer, to be transmitted to a specific node address. Received messages are optionally displayed on the OLED or can be retransmitted by UART Bluetooth to another paired phone or computer.

Here, node 1 reports a message received from node 2 at -57 dBm RSSI:

![IMG_1616](https://github.com/jremington/Feather-LoRa-Networks/assets/5509037/f3905397-d568-44e8-830d-529e5da48b65)
