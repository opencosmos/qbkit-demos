''' Base class for object which can be used in reactor '''

import os

class Reactant(object):
	''' Base class for object which can be used in reactor '''

	def __init__(self, fd, mode):
		self.fd = fd
		self.mode = mode

	def fileno(self):
		return self.fd

	def filemode(self):
		return self.mode

	def read(self, max_length):
		if not (self.filemode() & Reactant.READ):
			raise AssertionError("Not readable")
		return os.read(self.fileno(), max_length)

	def write(self, data):
		if not (self.filemode() & Reactant.WRITE):
			raise AssertionError("Not writeable")
		if not isinstance(data, bytes):
			raise TypeError("Input is not bytes")
		return os.write(self.fileno(), data)

Reactant.READ = 0x01
Reactant.WRITE = 0x02
Reactant.READWRITE = Reactant.READ | Reactant.WRITE
