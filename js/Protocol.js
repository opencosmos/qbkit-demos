const EventEmitter = require('events').EventEmitter;
const zmq = require('zeromq');

const terminator = 0;

const encode_label = str => Buffer.concat([Buffer.from(str, 'utf-8'), Buffer.from([terminator])]);

const decode_label = buf => {
	if (buf.length === 0 || buf[buf.length - 1] !== terminator) {
		return null;
	}
	return buf.subarray(0, buf.length - 1).toString('utf-8');
};

class Socket extends EventEmitter {
	constructor(config, logger = console) {
		super();
		this.logger = logger;
		this.host = config.host;
		const pub = zmq.socket('pub');
		const sub = zmq.socket('sub');
		this.pub = pub;
		this.sub = sub;
		this._init = [
			pub.connect(config.tx_url),
			sub.connect(config.rx_url)
		];
		sub.subscribe(encode_label(this.host));
		sub.on('message', (...parts) => this._recv(parts));
	}
	/* Returns void */
	async init() {
		await Promise.all(this._init);
	}
	/* Returns void */
	async send(envelope, ...parts) {
		const pub = this.pub;
		await pub.send(encode_label(envelope.remote), zmq.ZMQ_SNDMORE);
		await pub.send(encode_label(this.host), zmq.ZMQ_SNDMORE);
		await pub.send(encode_label(envelope.session), zmq.ZMQ_SNDMORE);
		await pub.send(encode_label(envelope.command), zmq.ZMQ_SNDMORE);
		for (let i = 0; i < parts.length; i++) {
			await pub.send(parts[i], i < parts.length - 1 ? zmq.ZMQ_SNDMORE : 0);
		}
	}
	/* Returns { envelope, parts } */
	_recv(msg) {
		if (msg.length < 4) {
			this.logger.warn('Insufficient message parts');
		}
		const [remote, host, session, command, ...parts] = msg;
		const envelope = new Envelope(
			decode_label(remote),
			decode_label(session),
			decode_label(command)
		);
		if (envelope.remote === null || envelope.session === null || envelope.command === null) {
			this.logger.warn('Invalid message envelope');
			return;
		}
		if (decode_label(host) !== this.host) {
			this.logger.warn('Received message addressed to some other host');
			return;
		}
		this.emit('message', envelope, ...parts);
	}
}

class Config {
	constructor(host = '', rx_url = '', tx_url = '', verbose = false) {
		this.host = host;
		this.rx_url = rx_url;
		this.tx_url = tx_url;
		this.verbose = verbose;
	}
}

class Envelope {
	constructor(remote = '', session = '', command = '') {
		if (arguments.length === 1 && typeof remote === 'object') {
			const o = remote;
			remote = o.remote;
			session = o.session;
			command = o.command;
		}
		this.remote = remote;
		this.session = session;
		this.command = command;
	}
}

Socket.Config = Config;
Socket.Envelope = Envelope;

module.exports = Socket;
