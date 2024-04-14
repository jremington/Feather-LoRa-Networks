**Simple example of AES128 encrypted TX and RX, working with Feather M0 Lora**

This is a broadcast message with encryption key in source code, TX to RX only (no node addresses, not bidirectional, no ACK/NACK for reliable data transfer). It demonstrates that the driver handles the block encryption properly, for an arbitrarily sized message.
