import asyncio
import zmq

class Config():
	rx_url = None
	tx_url = None
	remote = None
	hostname = None
	session = None
	timeout = None

class ZmqSocketError(RuntimeError):
	def __init__(self, msg):
		self.message = msg

class ReceiveTimeoutError(ZmqSocketError):
	pass

class InvalidMessageError(ZmqSocketError):
	pass

class OperationFailedError(ZmqSocketError):
	pass

class InvalidLabelError(ZmqSocketError):
	pass

class Envelope():
	remote = None
	session = None
	command = None

	def __init__(self, remote = None, session = None, command = None):
		self.remote = remote
		self.session = session
		self.command = command

terminator = 0

def encode_label(label):
	return bytes(label, 'utf-8') + bytes([terminator])

def decode_label(buf):
	if buf[-1] != terminator:
		raise InvalidLabelError('Invalid label')
	return str(buf[0:-1], 'utf-8')

class Socket():
	config = None
	ctx = None
	pub = None
	sub = None

	def __init__(self, ctx, config):
		self.config = config
		self.ctx = ctx
		self.pub = self.ctx.socket(zmq.PUB)
		self.sub = self.ctx.socket(zmq.SUB)
		self.pub.connect(config.tx_url)
		self.sub.connect(config.rx_url)
		self.sub.setsockopt(zmq.SUBSCRIBE, encode_label(config.hostname))

	def __enter__(self):
		self.ctx.__enter__();
		self.pub.__enter__();
		self.sub.__enter__();
		return self

	def __exit__(self, *args, **kwargs):
		self.sub.__exit__(*args, **kwargs);
		self.pub.__exit__(*args, **kwargs);
		self.ctx.__exit__(*args, **kwargs);
		return self

	async def send(self, envelope, *parts):
		msg = [
				encode_label(envelope.remote),
				encode_label(self.config.hostname),
				encode_label(envelope.session),
				encode_label(envelope.command),
		]
		msg += parts
		return await self.pub.send_multipart(msg)

	async def recv(self, timeout = None):
		if timeout is None:
			timeout = self.config.timeout
		if timeout is not None:
			timeout = timeout * 1000.0
		events = await self.sub.poll(timeout=timeout, flags=zmq.POLLIN)
		if not (events & zmq.POLLIN):
			raise ReceiveTimeoutError('Receive timed out')
		msg = await self.sub.recv_multipart()
		if len(msg) < 5:
			raise InvalidMessageError('Insufficient parts')
		try:
			local, remote, session, command = [decode_label(label) for label in msg[0:4]]
			if local != self.config.hostname:
				raise InvalidLabelError('Message is addressed to "' + local + '"')
		except InvalidLabelError as err:
			if timeout is None:
				return await recv(timeout)
			else:
				return ( None, None )
		parts = msg[4:]
		envelope = Envelope(remote, session, command)
		return ( envelope, parts )

Socket.Envelope = Envelope
