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
	topic = 'demo'
	max_read_size = 0x10000
	quiet = False

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
		# Deserialise
		try:
			msg = json.loads(str(packet, "utf-8"))
		except json.decoder.JSONDecodeError:
			if not self.config.quiet:
				print('Invalid JSON received, ignoring')
			return
		# Check type (request/response/?)
		type = msg.get('type')
		if type != 'request':
			if not self.config.quiet:
				print('Message receieved for a different communication type')
			return
		# Check topic
		topic = msg.get('topic')
		if topic != self.config.topic:
			if not self.config.quiet:
				print('Message receieved for a different topic, ignoring')
			return
		# Get command
		command = msg.get('command')
		if command is None:
			print('Error: No command specified')
			return
		# Get sequence value
		seq = msg.get('seq')
		if seq is None:
			print('Warning: No sequence value provided')
		# Get sequence value
		client = msg.get('client')
		if client is None:
			print('Warning: No client ID provided')
		# Get function for handling this command
		func = self.commands.get(command)
		if func is None:
			self.send_error(client, topic, command, seq, 'Unrecognised command')
			return
		# Get command parameters
		req = msg.get('data')
		if not self.config.quiet:
			print('Info: Executing command "' + command + '"')
		# Execute
		try:
			res = func(req)
		except BaseException as err:
			self.send_error(client, topic, command, seq, 'Command failed')
			if not self.config.quiet:
				print(err)
			return
		# Respond
		msg = {
			'type': 'response',
			'client': client,
			'topic': topic,
			'command': command,
			'seq': seq,
			'data': res
		}
		self.sock.sendto(bytes(json.dumps(msg), "utf-8"), (self.config.tx_host, self.config.tx_port))

	def send_error(self, client, topic, command, seq, error):
		print('Error: ' + str(error));
		msg = {
			'type': 'response',
			'client': client,
			'topic': topic,
			'command': command,
			'seq': seq,
			'error': error
		}
		self.sock.sendto(bytes(json.dumps(msg), "utf-8"), (self.config.tx_host, self.config.tx_port))

	def __enter__(self):
		self.sock.__enter__()
		return self

	def __exit__(self, *args, **kwargs):
		self.sock.__exit__(*args, **kwargs)
		return self

#################### DEMO / CLI STUFF COMES BELOW ####################

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
	def ping(self, data):
		return 'pong'
	def echo(self, data):
		return ' '.join(data)
	def upper(self, data):
		return ' '.join([x.upper() for x in data])
	def lower(self, data):
		return ' '.join([x.lower() for x in data])
	def date(self, data):
		return str(datetime.datetime.now())
	def help(self, data):
		return 'Commands available: ' + ', '.join(self.commands.keys())

class Program():
	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['tx_host=', 'tx_port=', 'rx_host=', 'rx_port=', 'max_read_size=', 'topic=', 'quiet'])

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
				elif opt in ('--topic'):
					config.topic = val
				elif opt in ('--quiet'):
					config.quiet = True
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size, config.topic, config.quiet):
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
		print('  ./udp_server.py')
		print('                  --tx_host=localhost --tx_port=5555')
		print('                  --rx_host=localhost --rx_port=5556')
		print('                  --topic=demo')
		print('                  --max_read_size=65536')
		print('                  --quiet')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
