#!/usr/bin/python3

''' Linux only.  Requires PySerial. '''

import os
import sys
import getopt
import fcntl
import select
import json
from serial import Serial
import socket
from asyncio import Queue

import Kiss

class Config():
	''' Configuration for UART/UDP bridge '''
	device = None
	baud = 9600
	server_host = 'localhost'
	server_port = 5555
	client_host = 'localhost'
	client_port = 5556
	ttl = 2
	max_read_size = 0x10000
	quiet = False

''' UART/UDP bridge implementation '''
class Bridge():

	# Note: we currently do not enforce a maximum size on the buffers.
	#
	# If you use this code in production, you definitely should set an upper
	# limit.

	# Buffer of bytes's from UDP to send to UART
	send_buf = Queue()

	# Buffer of packets from UART to send to UDP
	recv_buf = Queue()

	next_uart_write = None

	# Codec for packet access over character device
	encoder = Kiss.Encoder()
	decoder = Kiss.Decoder()

	def __init__(self, config):
		''' Main event loop '''
		self.config = config
		# Open serial port and socket
		with Serial(config.device, config.baud) as uart, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
			self.uart = uart
			self.sock = sock
			# Get file descriptors and configure them for non-blocking IO
			uart_fileno = uart.fileno()
			sock_fileno = sock.fileno()
			self.uart_fileno = uart_fileno
			self.sock_fileno = sock_fileno
			for fd in [uart_fileno, sock_fileno]:
				fcntl.fcntl(fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
			# Configure socket
			sock.bind((config.server_host, config.server_port))
			sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, config.ttl)
			while True:
				# Wait for UART or STDIN to become readable (TODO: conditionally add descriptors if send/recv buffer are not full)
				want_read = []
				want_read.append(uart_fileno)
				want_read.append(sock_fileno)
				# If we have data buffered to send, also wait for UART to become writeable
				want_write = []
				if not self.send_buf.empty():
					want_write.append(uart_fileno)
				if not self.recv_buf.empty():
					want_write.append(sock_fileno)
				# Wait for event
				r, w, e = select.select(want_read, want_write, [], 0)
				# Error
				if e:
					print('Error occurred')
					break
				# Read/write
				if sock_fileno in r:
					self.read_socket()
				if uart_fileno in w:
					self.write_uart()
				if uart_fileno in r:
					self.read_uart()
				if sock_fileno in w:
					self.write_socket()

	def read_socket(self):
		# Read packet from UDP
		try:
			packet, addr = self.sock.recvfrom(self.config.max_read_size)
		except BlockingIOError:
			print('Unexpected BlockingIOError while reading socket')
			return
		if not self.config.quiet:
			print("Packet of " + str(len(packet)) + " bytes received from " + str(addr))
		data = self.encoder.apply(packet)
		self.send_buf.put_nowait(data)

	def write_uart(self):
		# Write data from TX buffer then remove the amount written
		# from the buffer
		if self.next_uart_write is None:
			data = self.send_buf.get_nowait()
		else:
			data = self.next_uart_write
		written = os.write(self.uart_fileno, data)
		if written < len(data):
			self.next_uart_write = data[written:]
		else:
			self.next_uart_write = None

	def read_uart(self):
		try:
			data = os.read(self.uart_fileno, self.config.max_read_size)
		except BlockingIOError:
			print('Unexpected BlockingIOError while reading serial link, is another program using it?')
			return
		# Decode byte stream to packets using KISS decoder
		packets = self.decoder.apply(data)
		for packet in packets:
			if not self.config.quiet:
				print("Packet of " + str(len(packet)) + " bytes received from serial link")
			self.recv_buf.put_nowait(packet)

	def write_socket(self):
		packet = self.recv_buf.get_nowait()
		self.sock.sendto(packet, (self.config.client_host, self.config.client_port))

class Program():
	''' Wrapper to allow bridge to be invoked from command-line '''
	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		if not cmdline:
			self.usage()
			sys.exit(1)
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['device=', 'baud=', 'server_host=', 'server_port=', 'client_host=', 'client_port=', 'ttl=', 'max_read_size=', '--quiet'])

			for opt, val in opts:
				if opt in ('--device'):
					config.device = val
				elif opt in ('--baud'):
					config.baud = int(val)
				elif opt in ('--server_host'):
					config.server_host = val
				elif opt in ('--server_port'):
					config.server_port = int(val)
				elif opt in ('--client_host'):
					config.client_host = val
				elif opt in ('--client_port'):
					config.client_port = int(val)
				elif opt in ('--ttl'):
					config.ttl = int(val)
				elif opt in ('--max_read_size'):
					config.max_read_size = int(val)
				elif opt in ('--quiet'):
					config.quiet = True
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.device, config.baud, config.server_host, config.server_port, config.client_host, config.client_port, config.ttl, config.max_read_size):
				raise AssertionError('Required parameter missing')
			if args:
				raise AssertionError('Unexpected trailing arguments')
		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)
		Bridge(config)

	def usage(self):
		print('UART communications demo')
		print('')
		print('This program provides a UDP bridge for sending and receiving')
		print('data packets over a serial link')
		print('')
		print('It achieves a packet interface over a serial character interface by')
		print('utilising a packet encoding scheme from the KISS and SLIP protocols')
		print('')
		print('Syntax:')
		print('')
		print('  ./packet.py')
		print('              --device=/dev/ttyO1 --baud=9600')
		print('              --server_host=localhost --server_port=5555')
		print('              --client_host=localhost --client_port=5556')
		print('              --ttl=2')
		print('              --max_read_size=65536')
		print('              --quiet')
		print('')
		print('    --device=[value]         Path to serial device  ')
		print('    --baud=[value]           Baud rate of serial link')
		print('')
		print('    --server_host=[value]    Host and port to bind UDP server to (UDP -> serial)')
		print('    --server_port=[value]')
		print('')
		print('    --client_host=[value]    Host and port to send UDP packets to (serial -> UDP)')
		print('    --client_port=[value]')
		print('    --ttl=[value]            TTL value for packets sent to client (useful if sending to multicast group)')
		print('')
		print('    --max_read_size=[value]  Maximum amount of data to read in one operation, in bytes')
		print('')
		print('    --quiet                  Suppresses logging of each packet length and origin')
		print('')
		print('  Set the client host/port to the server host/port to create an echo server')
		print('')
		print('  The --device argument is required.  All others will default to the values shown above, if omitted.')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:])
