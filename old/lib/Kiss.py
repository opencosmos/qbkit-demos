''' Minimal KISS/SLIP framing/deframing '''

'''
KISS/SLIP protocol, provides packet interface over character interface

Commonly used by some amateur TNCs, and also to encapsulate IP packets over
serial links
'''

from collections import deque
from itertools import islice

from Functional import Consumer, Filter

# Singleton representing end of frame, emitted by KissDecoder
end_of_frame = object()

# Singleton representing that the current frame is invalid and has been
# terminated, emitted by KissDecoder
invalid_frame = object()

class KissConfig(object):
	''' Default KISS configuration '''
	FEND = 0xc0
	FESC = 0xdb
	TFEND = 0xdc
	TFESC = 0xdd

class KissEncoder(Consumer):
	''' Encoder, a consumer which encodes frames of bytes to a byte stream '''

	def __init__(self, consumer, auto_open=False, config=KissConfig()):
		''' Construct an encoder which acts as a consumer and outputs the
		encoded bytes to another consumer '''
		self.byte_encoder = _KissByteEncoder(config=config)
		self.consumer = consumer
		self.auto_open = auto_open

	def _assert_open(self):
		if not self.byte_encoder.is_open:
			if self.auto_open:
				self.open()
			else:
				raise AssertionError('Frame is not open')

	def _supply(self, bytes):
		for byte in bytes:
			self.consumer.accept(byte)

	def open(self):
		''' Opens a new frame '''
		self._supply(self.byte_encoder.open())

	def close(self):
		''' Closes the current frame '''
		self._supply(self.byte_encoder.close())

	def send_byte(self, data):
		''' Encodes a byte of data '''
		self._supply(self.byte_encoder.encode(data))

	def send_bytes(self, data, frame=True):
		''' Send a frame (opens/closes too if frame is true) '''
		if not isinstance(data, (bytes, bytearray)):
			raise TypeError('Invalid parameter type (expected bytes/bytearray)')
		if frame:
			self.open()
		try:
			for byte in data:
				self.send_byte(byte)
		finally:
			if frame:
				self.close()

	# Encode byte / bytes / Buffer
	def accept(self, data):
		''' Encode byte / bytes / bytearray (supply end_of_frame to close iff
		auto_open is set) '''
		if self.auto_open and data == end_of_frame:
			self.close()
		elif data == invalid_frame:
			# For testing only
			self._supply([self.byte_encoder.config.FESC])
			self._supply([self.byte_encoder.config.FESC])
		elif isinstance(data, (bytes, bytearray)):
			self.send_bytes(data)
		elif isinstance(data, int):
			self.send_byte(data)
		else:
			raise TypeError('Invalid parameter type (expect byte or bytes)')

	def heartbeat(self):
		''' Send a heartbeat (FEND) if no frame is open '''
		if self.byte_encoder.is_open:
			return
		self._supply(self.byte_encoder.idle())

class KissDecoder(Consumer):
	''' Decoder, consumes byte stream and produces either bytes or frames
	depending on emit_frames parameter '''

	def __init__(self, consumer, emit_frames=True, config=KissConfig):
		''' Construct a decoder which acts as a consumer and outputs the
		decoded frames to another consumer '''
		self.consumer = consumer
		self.byte_decoder = _KissByteDecoder(config=config)
		self.emit_frames = emit_frames
		self.buf = bytearray()

	def _supply(self, symbol):
		if symbol is None:
			return
		# If parameter is list, iterate over it
		if isinstance(symbol, (list, bytes, bytearray)):
			for sym in symbol:
				self._supply(sym)
			return
		# Parameter is single symbol
		if self.emit_frames:
			# Emit complete frames
			if symbol == end_of_frame:
				tmp = self.buf
				self.buf = bytearray()
				self.consumer.accept(bytes(tmp))
			elif symbol == invalid_frame:
				self.buf = bytearray()
				self.consumer.accept(invalid_frame)
			elif symbol is None:
				pass
			else:
				self.buf.append(symbol)
		else:
			# Emit decoded bytes and end_of_frame / invalid_frame
			self.consumer.accept(symbol)

	def accept(self, data):
		''' Process a single byte or a bytearray '''
		if isinstance(data, (bytes, bytearray)):
			for byte in data:
				self._supply(self.byte_decoder.decode(byte))
		elif isinstance(data, int):
			self._supply(self.byte_decoder.decode(data))
		else:
			raise TypeError('Invalid parameter type (expect byte or bytes)')

class KissFilter(Filter):
	''' KISS encoder/decoder filter '''

	def __init__(self, config=KissConfig()):
		self.config = config
		self.encoder = _KissByteEncoder(config=config)
		self.decoder = _KissByteDecoder(config=config)
		self.rxbuf = deque()

	def encode(self, frames):
		''' Encode some frames to bytes '''
		data = bytearray()
		if isinstance(frames[0], int):
			frames = [frames]
		for frame in frames:
			data += self.encoder.open()
			for byte in frame:
				data += self.encoder.encode(byte)
			data += self.encoder.close()
		return data

	def decode(self, data):
		''' Decode some bytes to frames '''
		decoded = []
		for item in data:
			if isinstance(item, int):
				# Byte
				decoded.append(self.decoder.decode(item))
			elif isinstance(item, (bytes, bytearray)):
				# Bytes/ByteArray
				for byte in item:
					decoded.append(self.decoder.decode(byte))
			else:
				raise TypeError('Invalid parameter type (expect byte/bytes/bytearray)')
		self.rxbuf += [byte for byte in decoded if byte is not None]
		# For convenience
		buffered = self.rxbuf
		# Extract frames
		begin = 0
		it = 0
		end = len(buffered)
		out = []
		while it < end:
			sym = buffered[it]
			it = it + 1
			if sym == invalid_frame:
				begin = it
			elif sym == end_of_frame:
				out.append(bytes(list(islice(buffered, begin, it-1))))
				begin = it
		# Remove processed data from buffer
		for i in range(0, begin):
			self.rxbuf.popleft()
		# How to return multiple frames at once?
		return out


class _KissByteEncoder(object):
	''' Encodes one byte at a time, returns bytes '''

	def __init__(self, config=KissConfig()):
		self.config = config
		self.is_open = False

	def open(self):
		if self.is_open:
			raise AssertionError('Frame is already open')
		self.is_open = True
		return bytes([self.config.FEND])

	def close(self):
		if not self.is_open:
			raise AssertionError('Frame is not open')
		self.is_open = False
		return bytes([self.config.FEND])

	def idle(self):
		return bytes([self.config.FEND])

	def encode(self, data):
		''' Encodes a byte of data '''
		if not isinstance(data, int):
			raise TypeError('Invalid parameter type (expected byte)')
		if data == self.config.FEND:
			return bytes([self.config.FESC, self.config.TFEND])
		elif data == self.config.FESC:
			return bytes([self.config.FESC, self.config.TFESC])
		else:
			return bytes([data])

class _KissByteDecoder(Consumer):
	''' Decodes one byte at a time, returns bytes/None/end_of_frame/invalid_frame '''

	def __init__(self, config=KissConfig):
		self.config = config
		self.previous_char = None
		self.is_open = False

	#pylint: disable=too-many-branches
	def decode(self, byte):
		''' Returns byte or invalid_frame / end_of_frame or None '''
		if not isinstance(byte, int):
			raise TypeError('Invalid parameter type (expected byte)')
		''' Result placeholder '''
		result = None

		if self.previous_char == self.config.FEND:
			if byte == self.config.FEND:
				# Multiple FENDs, do nothing
				pass
			else:
				# Previous byte was FEND, this one isn't - open frame
				self.is_open = True

		if self.is_open:
			# A frame is open, parse the byte as data
			if byte == self.config.FEND:
				if self.previous_char == self.config.FESC:
					# FEND after FESC, invalid
					self.is_open = False
					result = invalid_frame
				else:
					# Close the frame
					self.is_open = False
					result = end_of_frame
			elif self.previous_char == self.config.FESC:
				# Escaped byte
				if byte == self.config.TFESC:
					result = self.config.FESC
				elif byte == self.config.TFEND:
					result = self.config.FEND
				else:
					# Invalid escape sequence
					self.is_open = False
					result = invalid_frame
			elif byte == self.config.FESC:
				# Nothing
				pass
			else:
				# Verbatim
				result = byte
		# Update decoder state
		self.previous_char = byte
		# Return decoded data
		return result
