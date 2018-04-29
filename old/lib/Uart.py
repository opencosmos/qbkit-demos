''' Wrapper around Serial library, providing supplier/consumer interfaces '''

from serial import Serial

from Functional import Consumer, Supplier

from File import File

class Uart(File):
	''' Dumb wrapper for serial, providing consumer and supplier interfaces '''

	def __init__(self, path, baud=115200):
		self.impl = Serial(path, baud)
		super(Uart, self).__init__(self.impl.fileno(), owns_fd=False)

	def __enter__(self, *args):
		self.impl.__enter__()
		return self

	def __exit__(self, *args):
		self.impl.__exit__()
