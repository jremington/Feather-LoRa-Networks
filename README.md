# Feather-LoRa-Networks

This repository contains several working, tested examples of standalone LoRa networks based on the RadioHead library. They were developed and tested on the Adafruit Feather M0 LoRa module, but should work with any Feather radio that is supported by the RadioHead library.  

Examples include a self-organizing, standalone mesh network, based on the RHMesh manager, and fixed node repeater networks based on the RHRouter and RHReliableDatagram managers.

Message traffic features acknowledged, reliable message delivery to intermediate nodes, but in multi-hop networks, the ultimate target does not automatically acknowledge to the originator of successful delivery. In other words, end-to-end message receipt is not acknowledged. Another network layer or mechanism is needed for reliable end-to-end message delivery or, for example, file transfer.

In countless hours of tests, I have not encountered any obvious bugs in the RadioHead library. It is a solid, very reliable and very professional contribution to the community.

Some other posted and recommended open source examples of Feather mesh networks:
```
https://github.com/royyandzakiy/LoRa-RHMesh
https://nootropicdesign.com/projectlab/ ... etworking/
```
