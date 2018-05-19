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

class ExampleService():
	def __init__(self):
		None
	def ping(self, *args):
		return ['pong']
	def echo(self, *args):
		return args
	def upper(self, *args):
		return [x.upper() for x in args]
	def lower(self, *args):
		return [x.lower() for x in args]
	def date(self, *args):
		return str(datetime.datetime.now())
	def commands(self):
		return {
				'ping': self.ping,
				'echo': self.echo,
				'upper': self.upper,
				'lower': self.lower,
				'date': self.date
				}

class Server():
	example_msg = {
		'command': 'command name',
		'seq': 'sequence value, can be any type',
		'data': ['array', 'of', 'arguments']
	}
	def __init__(self, config, commands):
		with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
			sock.bind((config.rx_host, config.rx_port))
			while True:
				packet, addr = sock.recvfrom(config.max_read_size)
				try:
					msg = json.loads(str(packet, "utf-8"))
				except json.decoder.JSONDecodeError:
					print('Invalid JSON received, ignoring')
					continue
				try:
					command = msg['command']
					func = commands[command]
				except KeyError:
					print('Invalid command')
					continue
				try:
					seq = msg['seq']
				except KeyError:
					print('No sequence number specified')
				req = None
				try:
					req = msg['data']
				except KeyError:
					print('No request data specified')
				res = func(*req)
				msg = {
					'command': command,
					'seq': seq,
					'data': res
				}
				sock.sendto(bytes(json.dumps(msg), "utf-8"), (config.tx_host, config.tx_port))

class Program():
	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['tx_host=', 'tx_port=', 'rx_host=', 'rx_port=', 'max_read_size='])

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
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size):
				raise AssertionError('Required parameter missing')
			if args:
				raise AssertionError('Unexpected trailing arguments')
		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)
		Server(config, ExampleService().commands())

	def usage(self):
		print('Utility for handling requests over UDP.')
		print('')
		print('Intended to be used with the UDP/UART bridge.')
		print('')
		print('Syntax:')
		print('')
		print('  ./server.py')
		print('              --tx_host=localhost --tx_port=5555')
		print('              --rx_host=localhost --rx_port=5556')
		print('              --max_read_size=65536')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:])
