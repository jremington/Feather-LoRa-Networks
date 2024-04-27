Optional AES128 encrypted version of RHMesh with text messages on Serial1.  Works well.

Supports and logs message to the FeatherWing OLED display, if the option is enabled.

NOTE: If you're using ATmega328P, Atmega32u4, ATSAMD51 M4 or ATSAMD21 M0 Feather with FeatherWing
Button A is #9 (note this is also used for the battery voltage divider so if you want to use both make sure you **disable the pullup** when you use analogRead(), then turn on the pullup for button reads)

This example supports two nodes, change N_NODES and modify nodeId to support up to 10 nodes.

