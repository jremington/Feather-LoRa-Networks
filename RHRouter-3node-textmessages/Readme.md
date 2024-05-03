This implements a simple fixed node network with two clients and one repeater using RHRouter. Each client can accept text messages on Serial1 input,
and transmit those message reliably to the other client, via a repeater node. 

It is assumed that the connectivity is Client1 <-> Repeater2 <-> Client3 and is intended for cases where Client1 and Client3 are not line-of-sight 
to each other, but have line-of-sight connectivity with the repeater. End-to-end message reception is not guaranteed, but message transmission 
to the repeater is either silently acknowledged, or logged in case of failure.

When each client is initialized, it checks connectivity with the repeater and reports success of failure on the serial monitor.
The repeater simply forwards messages not directed to it and does not report this activity on the serial monitor.
