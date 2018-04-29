''' Wrapper around file, providing supplier/consumer interfaces '''

from Functional import Consumer, Supplier
from Reactant import Reactant

from select import select
import os
import fcntl

class File(Consumer, Supplier, Reactant):
	''' Dumb wrapper for serial '''

	owns_fd = None
	input_block_size = None

	def __init__(self, fd, input_block_size=None, owns_fd=True):
		Reactant.__init__(self, fd, Reactant.READWRITE)
		if not isinstance(owns_fd, bool):
			raise TypeError("Invalid parameter type")
		if not isinstance(input_block_size, int) and input_block_size is not None:
			raise TypeError("Invalid parameter type")
		self.owns_fd = owns_fd
		self.input_block_size = input_block_size

	def __enter__(self, *args):
		return self

	def __exit__(self, *args):
		if self.owns_fd:
			self.close()

	def fileno(self):
		return self.fd

	def _set_blocking(self, value):
		if not isinstance(value, bool):
			raise TypeError("Invalid parameter type")
		fd = self.fileno()
		fl = fcntl.fcntl(fd, fcntl.F_GETFL)
		if value:
			fcntl.fcntl(fd, fcntl.F_SETFL, fl & ~os.O_NONBLOCK)
		else:
			fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

	def close(self):
		if self.owns_fd:
			if self.fd >= 0:
				os.close(self.fd)
				self.fd = -1
		else:
			raise AssertionError("File descriptor is not owned by this instance")

	def read(self, length, timeout=None):
		if not isinstance(timeout, (float, int)) and timeout is not None:
			raise TypeError("Invalid parameter type")
		if timeout is None:
			self._set_blocking(True)
		else:
			self._set_blocking(False)
			if timeout > 0:
				r, w, e = select([self.fd], [], [], timeout)
				if not r:
					return 0
		return os.read(self.fd, length)

	def write(self, data, timeout=None):
		if not isinstance(data, bytes):
			raise TypeError("Invalid parameter type")
		if not isinstance(timeout, (float, int)) and timeout is not None:
			raise TypeError("Invalid parameter type")
		if timeout is None:
			self._set_blocking(True)
		else:
			self._set_blocking(False)
			if timeout > 0:
				r, w, e = select([], [self.fd], [], timeout)
				if not w:
					return 0
		return os.write(self.fd, data)

	def get(self, length=None, timeout=None):
		''' Supplier, reads byte/bytes from file '''
		if length is None:
			length = self.input_block_size
		if length is None:
			data = self.read(1)
			if data:
				return data[0]
			else:
				return None
		else:
			return self.read(length)

	def accept(self, data):
		''' Consumer, writes byte/bytes to file '''
		if data is None:
			return
		if isinstance(data, int):
			data = bytes([data])
		if not isinstance(data, bytes):
			raise TypeError("Invalid parameter type")
		self.write(data)
