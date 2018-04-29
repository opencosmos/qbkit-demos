''' Unit test for Uart module '''

import subprocess
import time

from Uart import Uart
from Buffer import Buffer
from Pipe import Pipe

proc = subprocess.Popen(['socat', 'PTY,link=uartA', 'PTY,link=uartB'])

# Race condition between socat and opening the UARTs
time.sleep(0.2)

try:

	# Data to send
	expect = b'potato'

	# Baud rate to use
	baud = 1200

	# Receive buffer
	actual = Buffer()

	# Open ends of serial link
	with Uart('uartA', baud) as uartA, Uart('uartB', baud) as uartB:

		# Write data
		for c in expect:
			uartA.accept(c)

		# Read data and pipe into receive buffer
		pipe = Pipe(uartB, actual)
		while pipe.run(timeout=0.1):
			pass


	# Assert received data is same as sent data
	expect = list(expect)
	actual = actual.data
	if actual == expect:
		print('UART test passed')
	else:
		print(expect)
		print(actual)
		raise AssertionError('UART loopback test failed')

finally:
	proc.terminate()
