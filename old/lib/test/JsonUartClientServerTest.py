''' Integration-test for Client & Server using JSON over UART '''

import subprocess
import time

from Server import Server
from Client import Client
from Json import JsonFilter
from Kiss import KissFilter
from Uart import Uart
from Functional import Consumer, Supplier, FilterChain

proc = subprocess.Popen(['socat', 'PTY,link=uartA', 'PTY,link=uartB'])

# Race condition between socat and opening the UARTs
time.sleep(0.2)

try:

	baud = 1200

	with Uart('uartA', baud) as uartClient, Uart('uartB', baud) as uartServer:

		filter=FilterChain([JsonFilter(), KissFilter()])

		server = Server(Consumer(), lambda x: x, filter=filter)
		client = Client(Consumer(), filter=filter, poll=lambda:None)

		server.backend = client
		client.backend = server

		data = {'Key': 'Potato', 'Value': ['Bigly!', 'Yuge!']}

		for x in range(1, 10):
			if client.request(data) != data:
				raise AssertionError('Client/Server echo test failed')

		print('Client/Server JSON-over-UART test passed')

finally:
	proc.terminate()
