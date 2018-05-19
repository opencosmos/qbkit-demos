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
	quiet = False

class ExampleService():
	def __init__(self):
		self.commands = {
			'ping': self.ping,
			'echo': self.echo,
			'upper': self.upper,
			'lower': self.lower,
			'date': self.date,
			'help': self.help
		}
	def ping(self, *args):
		return ['pong']
	def echo(self, *args):
		return args
	def upper(self, *args):
		return [x.upper() for x in args]
	def lower(self, *args):
		return [x.lower() for x in args]
	def date(self, *args):
		return [str(datetime.datetime.now())]
	def help(self, *args):
		return ['Commands available: ' + ', '.join(self.commands.keys())]

class Server():
	example_msg = {
		'seq': 'sequence value, can be any type',
		'command': 'command name',
		'data': ['array', 'of', 'arguments']
	}

	def __init__(self, config, commands):
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.config = config
		self.sock = sock
		self.commands = commands
		sock.bind((config.rx_host, config.rx_port))

	def handle_request(self):
		packet, addr = self.sock.recvfrom(self.config.max_read_size)
		try:
			msg = json.loads(str(packet, "utf-8"))
		except json.decoder.JSONDecodeError:
			print('Invalid JSON received, ignoring')
			return
		try:
			command = msg['command']
			func = self.commands[command]
		except KeyError:
			print('Invalid command')
			return
		try:
			seq = msg['seq']
		except KeyError:
			print('No sequence number specified')
			return
		req = None
		try:
			req = msg['data']
		except KeyError:
			print('No request data specified')
			return
		try:
			if not self.config.quiet:
				print('Executing command "' + command + '"')
			res = func(*req)
		except Exception as err:
			res = ['ERROR:', 'An error occurred']
			if not self.config.quiet:
				print('Command failed:')
				print(err)
			return
		msg = {
			'command': command,
			'seq': seq,
			'data': res
		}
		self.sock.sendto(bytes(json.dumps(msg), "utf-8"), (self.config.tx_host, self.config.tx_port))

	def __enter__(self):
		self.sock.__enter__()
		return self

	def __exit__(self, *args, **kwargs):
		self.sock.__exit__(*args, **kwargs)
		return self

class Program():
	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['tx_host=', 'tx_port=', 'rx_host=', 'rx_port=', 'max_read_size=', 'quiet'])

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
				elif opt in ('--quiet'):
					config.quiet = True
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size, config.quiet):
				raise AssertionError('Required parameter missing')
			if args:
				raise AssertionError('Unexpected trailing arguments')
		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)
		self.config = config

	def run(self):
		service = ExampleService()
		server = Server(self.config, service.commands)
		while True:
			server.handle_request()

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
		print('              --quiet')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
