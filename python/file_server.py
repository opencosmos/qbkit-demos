#!/usr/bin/python3

import os
import sys
import time
import base64
import getopt

import udp_server

class Config(udp_server.Config):
	def __init__(self):
		super(Config, self).__init__()
		self.topic = 'filesystem'
		self.quiet = True

class FileCacheEntry():
	file = None
	timeout = None
	deadline = None

	def __init__(self, file, timeout):
		self.file = file
		self.timeout = timeout
		self.kick()

	def kick(self):
		self.deadline = time.time() + self.timeout

class FileCache():
	files = dict()

	def __init__(self, timeout):
		self.timeout = timeout

	def _set(self, key, value):
		prev = self.files.get(key)
		if prev is not None:
			prev.file.close()
			del self.files[key]
		if value is not None:
			entry = FileCacheEntry(value, self.timeout)
			self.files[key] = entry

	def close(self, client, name):
		self._set((client, name), None)

	def open(self, client, name, path):
		file = open(path, "rb+")
		self._set((client, name), file)
		return file

	def create(self, client, name, path):
		file = open(path, "wb+")
		self._set((client, name), file)
		return file

	def get(self, client, name):
		entry = self.files[(client, name)]
		entry.kick()
		return entry.file

	def prune(self):
		deadline = time.time()
		to_remove = []
		for key, value in self.files.items():
			if value.deadline < deadline:
				value.file.close()
				to_remove.append(key)
		for key in to_remove:
			del self.files[key]

class FileSystemService():
	file_cache = FileCache(30)
	cwds = dict()
	initial_cwd = None

	def __init__(self):
		self.commands = {
			'ping': self.ping,
			'here': self.here,
			'enter': self.enter,
			'list': self.list,
			'open': self.open,
			'create': self.create,
			'close': self.close,
			'read': self.read,
			'write': self.write,
			'seek': self.seek,
			'tell': self.tell,
			'stat': self.stat,
			'help': self.help,
		}
		self.initial_cwd = os.getcwd()

	def ping(self, data, client):
		if data == 'help':
			return {
				'help': 'Simple connectivity test'
			}
		return 'pong'

	def switch_to_client(self, client):
		cwd = self.cwds.get(client)
		if cwd is None:
			cwd = self.initial_cwd
			self.cwds[client] = cwd
		os.chdir(cwd)

	def here(self, data, client):
		if data == 'help':
			return {
				'help': 'Show state information including current working directory and currently open files'
			}
		self.switch_to_client(client)
		cwd = os.getcwd()
		files = [key[1] for key in self.file_cache.files.keys() if key[0] == client]
		return {
			'path': cwd,
			'files': files
		}

	def enter(self, data, client):
		if data == 'help':
			return {
				'help': 'Change the current working directory, to the given "path" which may be absolute or relative to the current working directory'
			}
		self.switch_to_client(client)
		os.chdir(data['path'])
		self.cwds[client] = os.getcwd()
		return self.here(data, client)

	def list(self, data, client):
		if data == 'help':
			return {
				'help': 'List contents of the current working directory or a given "path" if specified.  Optionally "filter" using a Python regular expression.'
			}
		self.switch_to_client(client)
		filter = data.get('filter')
		if filter is None:
			filter = lambda x: True
		else:
			rx = re.compile(filter)
			filter = lambda x: rx.fullmatch(x)
		return {
			'list': [item for item in os.listdir(data.get('path')) if filter(item)]
		}

	def open(self, data, client):
		if data == 'help':
			return {
				'help': 'Open an existing file for read/write located at the given "path", identifying with the given "name"'
			}
		self.switch_to_client(client)
		self.file_cache.open(client, data['name'], data['path'])
		return self.stat(data, client)

	def create(self, data, client):
		if data == 'help':
			return {
				'help': 'Create/overwrite a file for read/write located at the given "path", identifying with the given "name"'
			}
		self.switch_to_client(client)
		self.file_cache.create(client, data['name'], data['path'])
		return self.stat(data, client)

	def close(self, data, client):
		if data == 'help':
			return {
				'help': 'Close an open file with the given "name"'
			}
		self.switch_to_client(client)
		self.file_cache.close(client, data['name'])
		return {
			'closed': data['name']
		}

	def read(self, data, client):
		if data == 'help':
			return {
				'help': 'Read "length" bytes from a previously-opened file with the given "name", returning the base64-encoded "data" which was read.'
			}
		self.switch_to_client(client)
		file = self.file_cache.get(client, data['name'])
		offset = data.get('offset')
		if offset is not None:
			file.seek(offset)
		data = str(base64.b64encode(file.read(data['length'])), 'ascii')
		return {
			'data': data
		}

	def write(self, data, client):
		if data == 'help':
			return {
				'help': 'Write the given base64-encoded "data" to a previously-opened file with the given "name", returning the actual "length" of data written.'
			}
		self.switch_to_client(client)
		file = self.file_cache.get(client, data['name'])
		offset = data.get('offset')
		if offset is not None:
			file.seek(offset)
		data = base64.b64decode(bytes(data['data'], 'ascii'))
		return {
			'length': file.write(data)
		}

	def seek(self, data, client):
		if data == 'help':
			return {
				'help': 'Seek a previously-opened file with the given "name" to the given "offset".  An optional "whence" parameter specifies whether this offset is \'absolute\' from start (or end if negative offset) or is \'relative\' to the current position.  Returns the new absolute "position".'
			}
		self.switch_to_client(client)
		file = self.file_cache.get(client, data['name'])
		offset = data['offset']
		whence = data.get('whence')
		if whence is not None:
			whence, offset = {
				'absolute': lambda x: (os.SEEK_SET, x) if x >= 0 else (os.SEEK_END, -x),
				'relative': lambda x: (os.SEEK_CUR, x)
			}[whence](offset)
		else:
			whence = os.SEEK_SET
		position = file.seek(offset, whence)
		return {
			'position': position
		}

	def tell(self, data, client):
		if data == 'help':
			return {
				'help': 'Return the current "position" in a previously-opened file with the given "name", in bytes.'
			}
		self.switch_to_client(client)
		file = self.file_cache.get(client, data['name'])
		return {
			'position': file.tell()
		}

	def stat(self, data, client):
		if data == 'help':
			return {
				'help': 'Returns stat info about the previously-opened file with the given "name".'
			}
		self.switch_to_client(client)
		file = self.file_cache.get(client, data['name'])
		(mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime) = os.fstat(file.fileno())
		return {
			'mode': mode,
			'inode': ino,
			'device': dev,
			'uid': uid,
			'gid': gid,
			'size': size,
			'mtime': mtime
		}

	def help(self, data, client):
		if data == 'help':
			return {
				'help': 'Show a command list'
			}
		try:
			self.switch_to_client(client)
			help = [
				'Commands available:'
			]
			for key, func in self.commands.items():
				help.append(' * ' + key + ' :: ' + func('help', None)['help'])
			return '\n'.join(help)
		except BaseException as err:
			print(err)

def show_usage(config=Config()):
	udp_server.show_usage(config)

#################### DEMO / CLI STUFF COMES BELOW ####################

class Program():
	def __init__(self, cmdline):
		if cmdline == ['--help']:
			self.usage()
			sys.exit(0)
		args = cmdline
		config, args = udp_server.parse_config(args, Config())
		if args:
			raise AssertionError('Unexpected trailing arguments')
		self.config = config

	def run(self):
		service = FileSystemService()
		server = udp_server.Server(self.config, service.commands)
		while True:
			server.handle_request() #timeout=1)
			service.file_cache.prune()

	def usage(self):
		print('File server for use over UDP/UART bridge and compatible interfaces')
		print('')
		print('Syntax:')
		print('')
		print('  ./file_server.py')
		show_usage()

if __name__ == '__main__':
	Program(sys.argv[1:]).run()
