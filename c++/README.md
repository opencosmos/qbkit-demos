# Build

	make

Outputs are in the `./bin` directory.

# Run

	# Serial / ØMQ bridge
	./bin/bridge --help

	# Demo chat program
	./bin/chat --help

# Test in loopback mode (no serial)

	# Bridge in one terminal
	./bin/bridge -v

	# Chat in another terminal
	./bin/chat

	# Send some messages, they should get echoed back

# Test over serial

	# At each end, start a bridge instance
	./bin/bridge --device=/dev/ttyS1 --baud=115200

	# At each end, start a chat instance
	./bin/chat

	# Send some messages from end to end

# Test over mock serial

	## Run each of the following commands in a separate pane/terminal

	# Create mock serial
	socat PTY,raw,echo=0,link=/tmp/ua PTY,raw,echo=0,link=/tmp/ub

	# Start bridge at "A" end of serial link
	./bin/bridge --device=/tmp/ua --tx_url=ipc:///var/tmp/ua_tx --rx_url=ipc:///var/tmp/ua_rx

	# Start bridge at "B" end of serial link
	./bin/bridge --device=/tmp/ub --tx_url=ipc:///var/tmp/ub_tx --rx_url=ipc:///var/tmp/ub_rx

	# Start chat and connect to bridge of "A" end of serial link
	./bin/chat --tx_url=ipc:///var/tmp/ua_tx --rx_url=ipc:///var/tmp/ua_rx --username=deadpool

	# Start chat and connect to bridge of "B" end of serial link
	./bin/chat --tx_url=ipc:///var/tmp/ub_tx --rx_url=ipc:///var/tmp/ub_rx --username=cable

This is automated using `tmux` in the `virtual_serial.sh` script.
