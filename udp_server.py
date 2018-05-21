#!/usr/bin/python3

import sys
import socket
import getopt
import json
import time
import datetime
import traceback

class Config(object):
	def __init__(self):
		self.tx_host = 'localhost'
		self.tx_port = 5555
		self.rx_host = 'localhost'
		self.rx_port = 5556
		self.topic = 'demo'
		self.max_read_size = 0x10000
		self.quiet = False

class RejectRequest(BaseException):
	def __init__(self, message):
		self.message = message

class Server():

	def __init__(self, config, commands):
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.config = config
		self.sock = sock
		self.commands = commands
		sock.bind((config.rx_host, config.rx_port))

	def handle_request(self, timeout = None):
		if timeout is None:
			self.sock.settimeout(None)
			return self.try_handle_request()
		else:
			deadline = time.time() + float(timeout)
			while time.time() < deadline:
				timeout = max(deadline - time.time(), 0)
				self.sock.settimeout(timeout)
				if self.try_handle_request():
					break

	def try_handle_request(self):
		try:
			packet, addr = self.sock.recvfrom(self.config.max_read_size)
		except socket.timeout:
			return False
		# Deserialise
		try:
			msg = json.loads(str(packet, "utf-8"))
		except json.decoder.JSONDecodeError:
			if not self.config.quiet:
				print('Invalid JSON received, ignoring')
			return False
		# Check type (request/response/?)
		type = msg.get('type')
		if type != 'request':
			if not self.config.quiet:
				print('Message receieved for a different communication type')
			return False
		# Check topic
		topic = msg.get('topic')
		if topic != self.config.topic:
			if not self.config.quiet:
				print('Message receieved for a different topic, ignoring')
			return False
		# Get command
		command = msg.get('command')
		if command is None:
			print('Error: No command specified')
			return False
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
			return False
		# Get command parameters
		req = msg.get('data')
		if not self.config.quiet:
			print('Info: Executing command "' + command + '"')
		# Execute
		try:
			res = func(req, client)
		except RejectRequest as err:
			self.send_error(client, topic, command, seq, err.message)
			if not self.config.quiet:
				print(err)
			return True
		except BaseException as err:
			self.send_error(client, topic, command, seq, 'Command failed')
			print('')
			traceback.print_exc()
			print('')
			return True
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
		return True

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

def parse_config(cmdline, initial=None):
	# Extract configuration from command line arguments, return remaining arguments
	if initial is None:
		config = Config()
	else:
		config = initial
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
			config.max_read_size = int(val, 0)
		elif opt in ('--topic'):
			config.topic = val
		elif opt in ('--quiet'):
			config.quiet = True
		else:
			raise AssertionError('Unhandled option: ' + opt)
	if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size, config.topic, config.quiet):
		raise AssertionError('Required parameter missing')
	return (config, args)

def show_usage(config=Config()):
	print('                  --tx_host=' + str(config.tx_host) + ' --tx_port=' + str(config.tx_port))
	print('                  --rx_host=' + str(config.rx_host) + ' --rx_port=' + str(config.rx_port))
	print('                  --topic=' + config.topic)
	print('                  --max_read_size=' + hex(config.max_read_size))
	print('                  --quiet')
	print('')

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
	def ping(self, data, client):
		return 'pong'
	def echo(self, data, client):
		return str(data)
	def upper(self, data, client):
		return str(data).upper()
	def lower(self, data, client):
		return str(data).lower()
	def date(self, data, client):
		return str(datetime.datetime.now())
	def help(self, data, client):
		return 'Commands available: ' + ', '.join(self.commands.keys())

class Program():
	def __init__(self, cmdline):
		if cmdline == ['--help']:
			self.usage()
			sys.exit(0)
		config, args = parse_config(cmdline)
		if args:
			raise AssertionError('Unexpected trailing arguments')
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
		show_usage()

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
