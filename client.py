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
	def __init__(self, config):
		with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
			if config.timeout is not None:
				sock.settimeout(config.timeout)
			sock.bind((config.rx_host, config.rx_port))
			seq = 0
			while True:
				args = input('$ ').split(' ')
				if not args:
					continue
				command = args[0]
				if command == 'quit':
					break
				cmdline = args[1:]
				msg = {
					'command': command,
					'seq': seq,
					'data': cmdline
				}
				seq = seq + 1
				sock.sendto(bytes(json.dumps(msg), "utf-8"), (config.tx_host, config.tx_port))
				packet, addr = sock.recvfrom(config.max_read_size)
				msg = json.loads(str(packet, "utf-8"))
				print(' '.join(msg['data']))

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
		Client(config)

	def usage(self):
		print('Utility for sending commands over UDP.')
		print('')
		print('Intended to be used with the UDP/UART bridge.')
		print('')
		print('Syntax:')
		print('')
		print('  ./client.py')
		print('              --tx_host=localhost --tx_port=5555')
		print('              --rx_host=localhost --rx_port=5556')
		print('              --max_read_size=65536')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:])
