#!/usr/bin/python3

''' Linux only.  Requires PySerial. '''

import os
import sys
import getopt
import fcntl
import select
import json
from serial import Serial

# SLIP/KISS magic bytes
FEND = 0xc0
FESC = 0xdb
TFEND = 0xdc
TFESC = 0xdd

# SLIP/KISS encoder
class KissEncoder(object):

	def __init__(self):
		None

	def apply(self, data):
		out = bytearray()
		out.append(FEND)
		for byte in data:
			if byte == FEND:
				out.append(FESC)
				out.append(TFEND)
			elif byte == FESC:
				out.append(FESC)
				out.append(TFENC)
			else:
				out.append(byte)
		out.append(FEND)
		return out

# SLIP/KISS decoder
class KissDecoder(object):

	packet = None
	escape = False
	error = False

	def __init__(self):
		None

	def apply(self, data):
		packets = []
		for byte in data:
			if self.packet is not None:
				if self.escape:
					if byte == TFEND:
						self.packet.add(FEND)
					elif byte == TFESC:
						self.packet.add(FESC)
					else:
						self.packet = None
						self.escape = False
						self.error = True
					self.escape = False
				elif byte == FESC:
					self.escape = True
				elif byte == FEND:
					packets.append(self.packet)
					self.packet = None
				else:
					self.packet.append(byte)
			elif byte != FEND and not self.error:
				self.packet = bytearray()
				self.packet.append(byte)
			elif byte == FEND:
				self.error = False
		return packets

class PacketDemo():

	# Buffer of data from STDIN to send to UART
	#
	# Note: we currently do not enforce a maximum size on this buffer.
	#
	# If you use this code in production, you definitely should set an upper limit,
	# and also use a proper queue instead of shifting an array.
	send_buf = bytearray()

	# Maximum amount of data to read at once
	BUFSIZE = 0x10000

	# Codec for packet access over character device
	encoder = KissEncoder()
	decoder = KissDecoder()

	# Sequence number (added for educational use, not needed for demo)
	seq = 1

	def usage(self):
		print('UART communications demo')
		print('')
		print('This program sends and receives JSON packets over a serial link')
		print('')
		print('It achieves a packet interface over a serial character interface by')
		print('utilising a packet encoding scheme from the KISS and SLIP protocols')
		print('')
		print('Syntax:')
		print('')
		print('The following three ways to invoke the demo are identical:')
		print('  ./packet.py --device=/dev/ttyO1 --baud=9600')
		print('  ./packet.py -d /dev/ttyO1 -b 9600')
		print('  ./packet.py /dev/ttyO1 9600')
		print('')
		print('Each of them specifies:')
		print(' (a) the path to the serial device')
		print(' (b) the baud rate')
		print('')
		print('On Linux, one may create a virtual serial port using the "socat"')
		print('utility:')
		print('')
		print('  socat PTY,link=/tmp/u1 PTY,link=/tmp/u2')
		print('')
		print('This creates a virtual serial port where one end is created with')
		print('path /dev/u1 and the other with path /tmp/u2')
		print('')
		print('One could then send messages along the virtual serial port by')
		print('opening two instances of this program in different terminals:')
		print('')
		print('  ./packet.py /tmp/ua 9600')
		print('  ./packet.py /tmp/ub 9600')
		print('')

	# Extract configuration from command line arguments
	def __init__(self, cmdline):
		device = None
		baud = None
		if not cmdline:
			self.usage()
			sys.exit(1)
		try:
			opts, args = getopt.getopt(cmdline, 'd:b:', ['device=', 'baud='])

			for opt, val in opts:
				if opt in ('-d', '--device'):
					device = val
				elif opt in ('-b', '--baud'):
					baud = int(val)
				else:
					raise AssertionError('Unhandled option: ' + opt)

			if device is None and baud is None and len(args) >= 2:
				[device, baud] = args[0:2]
				args = args[2:]

			if device is None or baud is None:
				raise AssertionError('Required parameter missing: shell <device> <baud> or shell --device=? --baud=?')

		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)

		self.device = device
		self.baud = baud

		# Encode arguments into TX buffer
		for arg in args:
			msg = {
				"seq": self.seq,
				"text": arg,
			}
			self.send_json(msg)

	def run(self):
		''' Main event loop '''
		# Open serial port
		with Serial(self.device, self.baud) as uart:
			# Get file descriptors and configure them for non-blocking IO
			uart_fileno = uart.fileno()
			stdin_fileno = sys.stdin.fileno()
			for fd in [stdin_fileno, uart_fileno]:
				fcntl.fcntl(fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
			while True:
				# Wait for UART or STDIN to become readable
				want_read = [uart_fileno, stdin_fileno]
				# If we have data buffered to send, also wait for UART to become writeable
				want_write = [uart_fileno] if self.send_buf else []
				# Wait for event
				r, w, e = select.select([uart_fileno, stdin_fileno], [uart_fileno], [], 0)
				# Error
				if e:
					print('Error occurred')
					break
				# Read from STDIN
				if stdin_fileno in r:
					# Form JSON packet from newly-read data
					text = os.read(stdin_fileno, self.BUFSIZE)
					msg = {
						"seq": self.seq,
						"text": str(text, "utf-8")
					}
					self.send_json(msg)
				# Write to UART
				if uart_fileno in w:
					# Write data from TX buffer then remove the amount written from
					# the buffer
					written = os.write(uart_fileno, self.send_buf)
					self.send_buf = self.send_buf[written:]
				# Read from UART
				if uart_fileno in r:
					data = os.read(uart_fileno, self.BUFSIZE)
					# Decode byte stream to packets using KISS decoder
					packets = self.decoder.apply(data)
					for packet in packets:
						# Decode JSON
						msg = json.loads(str(packet, "utf-8"))
						self.recv_json(msg)
	
	def send_json(self, msg):
		''' Call to send a JSON packet '''
		packet = bytes(json.dumps(msg), "utf-8")
		# Encode packet to KISS byte stream
		data = self.encoder.apply(packet)
		# Append KISS data to TX buffer
		self.send_buf += data
		# Bump sequence number
		self.seq = self.seq + 1

	def recv_json(self, msg):
		''' Called when receiving a JSON packet '''
		print("Packet received:")
		print("  Sequence: #" + str(msg["seq"]))
		print("  Message: " + msg["text"])
		print("")

if __name__ == '__main__':
	PacketDemo(sys.argv[1:]).run()
