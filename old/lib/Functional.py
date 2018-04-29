'''
Java-style functional interfaces, used to describe the interfaces
provided/expected by other classes
'''

class Consumer(object):
	''' Consumes data '''
	def accept(self, value):
		''' Called to provide data to the consumer [data -> None] '''
		raise NotImplementedError('Not implemented')

class Supplier(object):
	''' Produces data '''
	def get(self, value):
		''' Called to acquire data from the supplier [() -> data] '''
		raise NotImplementedError('Not implemented')

class Filter(object):
	''' Filter for transforming data on-the-fly [list -> list] '''

	def encode(self, data):
		''' Encode data '''
		raise NotImplementedError('Not implemented')

	def decode(self, data):
		''' Decode data '''
		raise NotImplementedError('Not implemented')

class FilterChain(Filter):
	''' Wrapper for a chain of filters '''

	def __init__(self, chain):
		self.chain = chain

	def encode(self, data):
		for filter in self.chain:
			data = filter.encode(data)
		return data

	def decode(self, data):
		for filter in reversed(self.chain):
			data = filter.decode(data)
		return data
