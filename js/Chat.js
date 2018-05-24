const readline = require('readline');
const getopt = require('node-getopt');

const Socket = require('./Protocol');

const opt = getopt.create([
	['', 'rx_url=ARG', 'URL of bridge RX endpoint'],
	['', 'tx_url=ARG', 'URL of bridge TX endpoint'],
	['', 'username=ARG', 'Username for chat session'],
	['', 'hostname=ARG', 'Hostname for chat program'],
	['h', 'help', 'Show help']
]).bindHelp().parseSystem();

if (opt.argv.length) {
	console.error('Invalid arguments', opt.argv);
	process.exit(1);
}

const hostname = opt.options.hostname || 'chat';
const username = opt.options.username || 'test';

const config = {
	host: hostname,
	rx_url: opt.options.rx_url || 'ipc:///var/tmp/serial_bridge_rx',
	tx_url: opt.options.tx_url || 'ipc:///var/tmp/serial_bridge_tx',
};

const rl = readline.createInterface({
	input: process.stdin,
	output: process.stdout,
});

const socket = new Socket(config);

socket.on('message', (envelope, ...parts) => {
	if (envelope.command !== 'message') {
		console.warn('Unrecognised message type');
		return;
	}
	if (parts.length !== 1) {
		console.warn('Unrecognised message type');
		return;
	}
	const text = parts[0].toString('utf-8');
	console.log(`[${envelope.session}] ${text}`);
});

rl.on('line', line => {
	const envelope = {
		remote: hostname,
		session: username,
		command: 'message'
	};
	socket.send(envelope, Buffer.from(line, 'utf-8'));
});
