'''
Client: companion for Server class, sends request to Server and receives
response.
'''

from Buffer import Buffer
from Pipe import Pipe
from Functional import Consumer, Supplier, Filter, FilterChain

class RequestFailedError(RuntimeError):
	''' Exception type for failed requests '''
	pass

class Client(Consumer, Supplier):
	'''
	Client class, requires a consumer/supplier hybrid for the communications
	backend, which accepts and provides packets ("bytes")
	'''

	def __init__(self, backend, poll=None, filter=FilterChain([])):
		''' The backend is a consumer/supplier hybrid for communicating.
		Alternatively, a "poll" function can be provided, which is called by
		"request" between sending and receiving, and which should supply data
		to this instance via the "accept" method. '''
		self.backend = backend
		self.buf = Buffer()
		self.filter = filter
		self.poll = poll
		if filter is not None and not isinstance(filter, Filter):
			raise TypeError('The filter must implement the Filter interface')
		if not isinstance(backend, Consumer):
			raise TypeError('Backend must implement the Consumer interface')
		if isinstance(backend, Supplier):
			self.pipe = Pipe(backend, self.buf)
		else:
			self.pipe = None
		if self.poll is not None == self.pipe is not None:
			raise AssertionError('Either the backend must implement the supplier interface, xor the "poll" callback must be specified')

	def _supply(self, packets):
		''' Emits a packet to the backend '''
		data = self.filter.encode(packets)
		for item in data:
			self.backend.accept(item)

	def request(self, req, *args, **kwargs):
		''' Send a request, return the response.  Forwards extra arguments to
		the backend supplier. '''
		self.buf.data = []
		self._supply([req])
		if self.poll is not None:
			self.poll()
		else:
			self.pipe.run(*args, **kwargs)
		if not self.buf.data:
			raise RequestFailedError('Request failed, no response received')
		data = self.filter.decode(self.buf.data)
		res = data.pop()
		self.buf.data = []
		return res

	def accept(self, data):
		''' Can be used from within the "poll" callback to provide data, if the
		backend is not bi-directional '''
		self.buf.accept(data)
