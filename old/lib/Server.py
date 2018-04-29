'''
Server: receives commands, executes them synchronously and sends responses

Not using asyncio, as I want this to be more intuitive for beginners.
'''

from collections import deque

from Buffer import Buffer
from Pipe import Pipe
from Functional import Consumer, Supplier, Filter, FilterChain

class Server(Consumer):
	'''
	Server class, requires a consumer/supplier hybrid for the communications
	backend, which accepts and provides packets ("bytes").

	Exposes Consumer interface which can be used instead of this receive method.
	If the receive method is not used, then the backend can be just a consumer.
	'''

	def __init__(self, backend, handler, filter=FilterChain([])):
		''' The backend is a consumer/supplier hybrid for communicating, and
		the handler is a function which accepts a frame ("bytes") and returns a
		frame ("bytes").  '''
		self.backend = backend
		self.handler = handler
		self.filter = filter
		self.buf = Buffer(deque)
		if filter is not None and not isinstance(filter, Filter):
			raise TypeError('The filter must implement the Filter interface')
		if not isinstance(backend, Consumer):
			raise TypeError('Backend must implement the Consumer interface')
		if isinstance(backend, Supplier):
			self.pipe = Pipe(backend, self.buf)
		else:
			self.pipe = None

	def _supply(self, packets):
		''' Emits packets to the backend '''
		data = self.filter.encode(packets)
		for item in data:
			self.backend.accept(item)

	def _consume(self):
		''' Reads packets from RX queue '''
		return self.filter.decode(self.buf.take())

	def _handle_messages(self):
		''' Handles/executes any messages in the buffer '''
		reqs = self._consume()
		for req in reqs:
			res = self.handler(req)
			if res is not None:
				self._supply([res])

	def receive(self, *args, **kwargs):
		''' Attempts to receive data into RX queue, forwarding any arguments to
		the backend supplier.  Received packet(s) (if any) are then executed.
		'''
		if self.pipe is None:
			raise TypeError('Backend does not implement Supplier interface')
		self.pipe.run(*args, **kwargs)
		self._handle_messages()

	def accept(self, data):
		''' Alternative to using a bi-directional backend and receive: this
		Consumer interface can be used for reception instead.  Packet(s) (if
		any) are executed.  '''
		self.buf.accept(data)
		self._handle_messages()
