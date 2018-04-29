import os
import sys
import getopt
from serial import Serial
from asyncio import Queue
import fcntl
import select

def configure(args):
	device = None
	baud = None
	try:
		opts, args = getopt.getopt(args, 'd:b:', ['device=', 'baud='])

		for opt, val in opts:
			if opt in ('-d', '--device'):
				device = val
			elif opt in ('-b', '--baud'):
				baud = val
			else:
				raise AssertionError('Unhandled option: ' + opt)

		if device is None and baud is None and len(args) >= 2:
			[device, baud] = args[0:2]
			args = args[2:]

		if device is None or baud is None:
			raise AssertionError('Required parameter missing: shell <device> <baud> [<cmd-args>...] or shell --device=? --baud=? [<cmd-args>...]')

	except getopt.GetoptError as err:
		print(err)
		usage()
		sys.exit(1)

	return ( device, baud, args )

def splice(fi, fo):
	BUFSIZE = 0x10000
	data = os.read(fi, BUFSIZE)
	if data:
		os.write(fo, data)

def run(device, baud, args):
	with Serial(device, baud) as uart:
		uart_fileno = uart.fileno()
		stdin_fileno = sys.stdin.fileno()
		stdout_fileno = sys.stdout.fileno()
		for fd in [stdin_fileno, uart_fileno, stdout_fileno]:
			fcntl.fcntl(fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
		while True:
			r, w, e = select.select([uart_fileno, stdin_fileno], [uart_fileno, stdout_fileno], [], 0)
			if e:
				print('Error occurred')
				break
			if uart_fileno in r and stdout_fileno in w:
				splice(uart_fileno, stdout_fileno)
			if stdin_fileno in r and uart_fileno in w:
				splice(stdin_fileno, uart_fileno)
		proc.wait()

def main():
	device, baud, args = configure(sys.argv[1:])
	run(device, baud, args)

if __name__ == '__main__':
	main()
