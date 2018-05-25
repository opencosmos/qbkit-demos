#!/usr/bin/python3

import asyncio
import sys
import getopt
import aioconsole
import zmq.asyncio

from Protocol import Socket, InvalidMessageError, OperationFailedError, ReceiveTimeoutError

class Config():
	rx_url = 'ipc:///var/tmp/serial_bridge_rx'
	tx_url = 'ipc:///var/tmp/serial_bridge_tx'
	remote = 'chat'
	hostname = 'chat'
	session = 'test'
	timeout = 0.2

#################### DEMO / CLI STUFF COMES BELOW ####################

class Program():

	exit = False
	next = True

	def __init__(self, cmdline):
		# Extract configuration from command line arguments
		config = Config()
		try:
			opts, args = getopt.getopt(cmdline, '', ['tx_url=', 'rx_url=', 'topic=', 'hostname='])

			for opt, val in opts:
				if opt in ('--tx_url'):
					config.tx_url = val
				elif opt in ('--rx_url'):
					config.rx_url = val
				elif opt in ('--remote'):
					config.remote = val
				elif opt in ('--session'):
					config.session = val
				elif opt in ('--hostname'):
					config.hostname = val
				else:
					raise AssertionError('Unhandled option: ' + opt)
			if None in (config.tx_url, config.rx_url, config.remote, config.session, config.hostname):
				raise AssertionError('Required parameter missing')
			if args:
				raise AssertionError('Unexpected trailing arguments')
		except (getopt.GetoptError, ValueError) as err:
			print(err)
			self.usage()
			sys.exit(1)
		self.config = config

	async def run(self):
		ctx = zmq.asyncio.Context(4)
		with Socket(ctx, self.config) as socket:
			await asyncio.wait([fn(socket) for fn in [self._receiver, self._sender]])

	async def _receiver(self, socket):
		while not self.exit:
			await self._receiver_loop(socket)

	async def _sender(self, socket):
		while not self.exit:
			if not await self._sender_loop(socket):
				self.exit = True

	async def _receiver_loop(self, socket):
		# Receive message
		envelope, parts = (None, None)
		try:
			envelope, parts = await socket.recv(timeout=None)
		except InvalidMessageError as err:
			print('(Received invalid response from server: ' + err.message + ')')
			print('')
		except OperationFailedError as err:
			print('(Operation failed: ' + err.message + ')')
			print('')
		except ReceiveTimeoutError as err:
			pass
		if envelope is None:
			return
		if envelope.command != 'message':
			print('(Unknown command: "' + envelope.command + '")')
			return
		# Print message
		for part in parts:
			print('[' + envelope.session + '] ' + str(part, 'utf-8'))
		# Indicate to input task that message has been received
		self.next = True

	async def _sender_loop(self, socket):
		envelope = Socket.Envelope(self.config.remote, self.config.session, 'message')
		try:
			msg = (await aioconsole.ainput(''))
			# Unset flag which is used to notify that a message has been received
			self.next = False
			# Send request
			await socket.send(envelope, bytes(msg, 'utf-8'))
			# Wait up to 1s for reply, so terminal doesn't get cluttered as easily
			for i in range(0, 10):
				if self.next:
					break
				await asyncio.sleep(0.1)
		except EOFError:
			return False
		except BaseException as err:
			print('Unexpected exception:')
			print(str(err))
			return False
		return True

	def usage(self):
		print('Utility for sending commands over UDP.')
		print('')
		print('Intended to be used with the UDP/UART bridge.')
		print('')
		print('Syntax:')
		print('')
		print('  ./udp_client.py')
		print('                  --tx_host=' + config.tx_host + ' --tx_port=' + int(config.tx_port))
		print('                  --rx_host=' + config.rx_host + ' --rx_port=' + int(config.rx_port))
		print('                  --remote=' + config.remote)
		print('                  --session=' + config.session)
		print('')

if __name__ == '__main__':
	zmq.asyncio.install()
	prog = Program(sys.argv[1:])
	loop = asyncio.get_event_loop()
	loop.run_until_complete(prog.run())
	loop.close()

