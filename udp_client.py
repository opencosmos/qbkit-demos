#!/usr/bin/python3

import sys
import socket
import getopt
import json
import datetime

class Config():
	tx_host = 'localhost'
	tx_port = 5555
	rx_host = 'localhost'
	rx_port = 5556
	max_read_size = 0x10000
	timeout = 1

class Client():
	sock = None
	config = None
	seq = 0

	def __init__(self, config):
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.config = config
		self.sock = sock
		if config.timeout is not None:
			sock.settimeout(config.timeout)
		sock.bind((config.rx_host, config.rx_port))

	def __enter__(self):
		self.sock.__enter__()
		return self

	def __exit__(self, *args, **kwargs):
		self.sock.__exit__(*args, **kwargs)
		return self

	def request(self, command, data, timeout=None):
		if timeout is None:
			timeout = self.config.timeout
		seq = self.seq
		self.seq = self.seq + 1
		req = {
			'seq': seq,
			'command': command,
			'data': data
		}
		self.sock.sendto(bytes(json.dumps(req), "utf-8"), (self.config.tx_host, self.config.tx_port))
		packet, addr = self.sock.recvfrom(self.config.max_read_size)
		res = json.loads(str(packet, "utf-8"))
		if res['seq'] != seq:
			raise AssertionError('Sequence number mismatch')
		if res['command'] != command:
			raise AssertionError('Command mismatch')
		return res['data']

class Program():
	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['tx_host=', 'tx_port=', 'rx_host=', 'rx_port=', 'max_read_size=', 'timeout='])

			for opt, val in opts:
				if opt in ('--tx_host'):
					config.tx_host = val
				elif opt in ('--tx_port'):
					config.tx_port = int(val)
				elif opt in ('--rx_host'):
					config.rx_host = val
				elif opt in ('--rx_port'):
					config.rx_port = int(val)
				elif opt in ('--max_read_size'):
					config.max_read_size = int(val)
				elif opt in ('--timeout'):
					config.timeout = float(val)
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size, config.timeout):
				raise AssertionError('Required parameter missing')
			if args:
				raise AssertionError('Unexpected trailing arguments')
		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)
		self.config = config

	def run(self):
		with Client(self.config) as client:
			while True:
				args = input('$ ').split(' ')
				if not args:
					continue
				command = args[0]
				if command == 'quit':
					break
				req = args[1:]
				res = client.request(command, req)
				print(' '.join(res))

	def usage(self):
		print('Utility for sending commands over UDP.')
		print('')
		print('Intended to be used with the UDP/UART bridge.')
		print('')
		print('Syntax:')
		print('')
		print('  ./udp_client.py')
		print('                  --tx_host=localhost --tx_port=5555')
		print('                  --rx_host=localhost --rx_port=5556')
		print('                  --max_read_size=65536')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
