''' Provides the Buffer class, a bytearray Consumer wrapper '''

from Functional import Consumer

class Buffer(Consumer):
	''' Wraps a container in the Consumer interface, with special treatment for
	bytearray (see accept method doc). '''

	def __init__(self, buffer_type=list):
		self.buffer_type = buffer_type
		self.reset()

	def __iter__(self):
		return self.data.__iter__()

	def accept(self, data):
		''' If buffer_type is bytearray, then this accepts a byte, bytes, or a
		bytearray, and appends/joins the data to the buffer.  Otherwise, the
		append method is used on the buffer object. '''
		if isinstance(self.data, bytearray):
			if isinstance(data, (bytearray, bytes)):
				self.data += data
			elif isinstance(data, int):
				self.data.append(data)
			else:
				raise TypeError('Invalid parameter type (expected byte/bytes/bytearray)')
		else:
			self.data.append(data)

	def reset(self):
		''' Replace data with new empty bytearray '''
		self.data = self.buffer_type()

	def take(self):
		tmp = self.data
		self.reset()
		return tmp
