#!/usr/bin/python3

import sys
import socket
import getopt
import json
import datetime
import time
import secrets

class Config():
	tx_host = 'localhost'
	tx_port = 5555
	rx_host = 'localhost'
	rx_port = 5556
	max_read_size = 0x10000
	topic = 'demo'
	timeout = 1

class UdpClientError(RuntimeError):
	def __init__(self, msg):
		self.message = msg

class RequestTimeoutError(UdpClientError):
	pass

class InvalidResponseError(UdpClientError):
	pass

class OperationFailedError(UdpClientError):
	pass

class Client():
	sock = None
	config = None
	seq_prefix = None
	seq = 0

	def __init__(self, config):
		self.seq_prefix = secrets.token_urlsafe(22)
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.config = config
		self.sock = sock
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
		self.sock.settimeout(self.config.timeout)
		deadline = time.time() + float(timeout)
		seq = self.seq
		self.seq = self.seq + 1
		seq_str = self.seq_prefix + str(seq)
		topic = self.config.topic
		req = {
			'type': 'request',
			'topic': topic,
			'seq': seq_str,
			'command': command,
			'data': data
		}
		self.sock.sendto(bytes(json.dumps(req), "utf-8"), (self.config.tx_host, self.config.tx_port))
		do_while = True
		while do_while or time.time() < deadline:
			do_while = False
			try:
				packet, addr = self.sock.recvfrom(self.config.max_read_size)
			except socket.timeout:
				break
			res = json.loads(str(packet, "utf-8"))
			if res.get('type') != 'response':
				continue
			if res.get('topic') != topic:
				continue
			if res.get('seq') != seq_str:
				continue
			if res.get('command') != command:
				raise InvalidResponseError('Command mismatch')
			err = res.get('error')
			if err:
				raise OperationFailedError(err)
			data = res.get('data')
			if data is None:
				raise InvalidResponseError('No data in response')
			return data
		raise RequestTimeoutError('Timed out while waiting for response')

#################### DEMO / CLI STUFF COMES BELOW ####################

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
				elif opt in ('--topic'):
					config.topic = val
				elif opt in ('--timeout'):
					config.timeout = float(val)
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_host, config.tx_port, config.rx_host, config.rx_port, config.max_read_size, config.topic, config.timeout):
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
				cmdline = input('$ ').strip()
				if not cmdline or cmdline[0] == '#':
					continue;
				args = cmdline.split(' ')
				command = args[0]
				req = args[1:]
				try:
					if command == 'topic':
						if len(req) == 0:
							print('(Topic is "' + self.config.topic + '")')
						elif len(req) == 1:
							self.config.topic = req[0]
						else:
							print('(Syntax error: topic [<topic-name>])')
					elif command == 'quit':
						print('(Quitting)')
						break
					else:
						res = client.request(command, req)
						print(res)
					if command == 'help':
						print('(Extra client-side commands: topic, quit)')
				except InvalidResponseError as err:
					print('(Received invalid response from server: ' + err.message + ')')
					print('')
				except OperationFailedError as err:
					print('(Operation failed: ' + err.message + ')')
					print('')
				except RequestTimeoutError as err:
					print('(Timed out while waiting for response)')
					print('')
				except BaseException as err:
					print('Unexpected exception:')
					print(str(err))
					break

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
		print('                  --topic=demo')
		print('                  --max_read_size=65536')
		print('')

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
