# SLIP/KISS magic bytes
FEND = 0xc0
FESC = 0xdb
TFEND = 0xdc
TFESC = 0xdd

# SLIP/KISS encoder
class Encoder(object):

	def __init__(self):
		None

	def apply(self, data):
		out = bytearray()
		out.append(FEND)
		for byte in data:
			if byte == FEND:
				out.append(FESC)
				out.append(TFEND)
			elif byte == FESC:
				out.append(FESC)
				out.append(TFENC)
			else:
				out.append(byte)
		out.append(FEND)
		return out

# SLIP/KISS decoder
class Decoder(object):

	packet = None
	escape = False
	error = False

	def __init__(self):
		None

	def apply(self, data):
		packets = []
		for byte in data:
			if self.packet is not None:
				if self.escape:
					if byte == TFEND:
						self.packet.add(FEND)
					elif byte == TFESC:
						self.packet.add(FESC)
					else:
						self.packet = None
						self.escape = False
						self.error = True
					self.escape = False
				elif byte == FESC:
					self.escape = True
				elif byte == FEND:
					packets.append(self.packet)
					self.packet = None
				else:
					self.packet.append(byte)
			elif byte != FEND and not self.error:
				self.packet = bytearray()
				self.packet.append(byte)
			elif byte == FEND:
				self.error = False
		return packets
