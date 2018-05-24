# qbkit-demos

This repository demonstrates how to achieve packet communication over a serial link.

While it is implemented with horrendous overhead using Python / JSON, the underlying logic can be easily squeezed into small MCUs if re-written in C and using compact binary protocols.

The underlying KISS/SLIP protocol (which is used to serialise/deserialise packets over a character interface) is incredibly simple, requiring no state for the encoder and (at minimum) only a couple of bits for the decoder.

The Python demos require Linux, Python 3.x, PySerial.

## udp_bridge

For command-line syntax:

	./udp_bridge --help

This program provides a UDP socket interface to a serial port.

It first opens and configures the serial link (--device/--baud).
It listens on one UDP socket (--tx_host/--tx_port) for datagrams which are then transmitted over the serial link.
Datagrams received from the serial link are sent to another UDP socket (--rx_host/--rx_port).

In order to share the bridge with multiple UDP programs, you will need to implement UDP multicasting.

## udp_client / udp_server

Example client and server packet-based request/response communications model over serial link.

When starting either end, give it the relevant socket configuration to talk to the `udp_bridge` (--tx_host/--tx_port/--rx_host/--rx_port).

You can test these using a mock serial-link on Linux:

	### Run each command in separate terminal/tab
	
	# Create mock serial link (ends are at /tmp/ua and /tmp/ub)
	socat PTY,link=/tmp/ua PTY,link=/tmp/ub
	
	# Initiate bridge on one side of link using ports 5000/5001
	./udp_bridge.py --device=/tmp/ua --tx_port=5000 --rx_port=5001
	
	# Initiate bridge on other side of link using ports 6000/6001
	./udp_bridge.py --device=/tmp/ub --tx_port=6000 --rx_port=6001
	
	# Initiate server, configuring it to use the bridge on the /tmp/ua side of the link
	./udp_server.py --tx_port=5000 --rx_port=5001
	
	# Initiate client, configuring it to use the bridge on the /tmp/ub side of the link
	./udp_client.py --tx_port=6000 --rx_port=6001
	
	### Now in the client, you can run:
	help - to get list of commands available (from the server)
	ping - to ping the server
	date - to get the date/time on the server
	(and a few others listed in the "help" output)

## file_server

A file-server demo is also available.

Run the previous set of commands, excluding the `./udp_server` command.

Instead, start the file server (which uses udp_server internally):

	./file_server.py --tx_port=5000 --rx_port=5001

Create a test file:

	echo hello > /tmp/x
	cat /tmp/x
	hello

Now in the client, switch to `fileserver` topic:

	topic fileserver

Now you can interact with the filesystem via the file server, by sending commands with JSON parameters:

	open {"name":"test","path":"/tmp/x"}
	read {"name":"test","length":1000}
	seek {"name":"test","offset":4}
	write {"name":"test","data":"fire\n"}
	close {"name":"test"}

Now you can verify the changes in bash:

	cat /tmp/x
	hellfire

