'''
JSON serialiser and deserialiser consumers
'''

import json

from Functional import Consumer, Filter

class JsonSerialiser(Consumer):
	''' Accepts JSON structures, emits byte packets (bytes) to consumer '''

	def __init__(self, consumer, encoding='utf-8'):
		self.consumer = consumer
		self.filter = JsonFilter(encoding)

	def accept(self, data):
		''' Convert python data structure to UTF-8 JSON bytes '''
		self.consumer.accept(self.filter.encode([data])[0])

class JsonDeserialiser(Consumer):
	''' Accepts packets (bytes), emits JSON structures '''

	def __init__(self, consumer, encoding='utf-8'):
		self.consumer = consumer
		self.filter = JsonFilter(encoding)

	def accept(self, data):
		''' Convert UTF-8 JSON packets to python data structure '''
		self.consumer.accept(self.filter.decode([data])[0])

class JsonFilter(Filter):
	''' Json encoder/decoder filter '''

	def __init__(self, encoding='utf-8'):
		self.encoding = encoding
	
	def _encode_packet(self, packet):
		if self.encoding is None:
			return json.dumps(packet)
		else:
			return json.dumps(packet).encode(self.encoding)
	
	def _decode_packet(self, packet):
		if isinstance(packet, (bytes, bytearray)):
			if self.encoding is None:
				raise AssertionError('Encoding has not been specified')
			else:
				return json.loads(packet.decode(self.encoding))
		elif isinstance(data, str):
			return json.loads(data)

	def encode(self, packets):
		return [self._encode_packet(packet) for packet in packets]

	def decode(self, packets):
		return [self._decode_packet(packet) for packet in packets]
