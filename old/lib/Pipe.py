''' Helper class for piping from a supplier to a consumer '''

from Functional import Supplier, Consumer

class Pipe(object):
	''' Helper class for piping from a supplier to a consumer '''

	def __init__(self, supplier, consumer):
		if not isinstance(supplier, Supplier):
			raise TypeError('The supplier must implement the Supplier interface')
		if not isinstance(consumer, Consumer):
			raise TypeError('The consumer must implement the Consumer interface')
		self.supplier = supplier
		self.consumer = consumer

	def run(self, *args, **kwargs):
		''' Forwards aguments to supplier, if supplier returns anything except
		None then the return value is passed to the consumer '''
		value = self.supplier.get(*args, **kwargs)
		if value is not None:
			self.consumer.accept(value)
		return value is not None
