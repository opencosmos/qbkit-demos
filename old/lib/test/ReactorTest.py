'''
Test the reactor, using two pipes chained together
'''

import os

from Reactor import Reactor
from File import File

out = []

fd_r, fd_w = os.pipe()

pw = File(fd_w)
pr = File(fd_r)

data_in = [b'hello', b' ', b'world', b'!']
data_out = []

expect = bytes.join(b'', data_in)

data_in.reverse()

done = []

reactor = Reactor()

def pw_io(e):
	data = data_in.pop()
	pw.write(data, timeout=0.1)
	if not data_in:
		reactor.remove(pw)
		pw.close()

def pr_io(e):
	data = pr.read(100, timeout=0.1)
	if not len(data):
		done.append(True)
	else:
		data_out.append(data)

reactor.add(pw, pw_io, Reactor.WRITE)
reactor.add(pr, pr_io, Reactor.READ)

while not done:
	if not reactor.react(0.1):
		raise AssertionError("Timeout")

pw.close()
pr.close()

actual = bytes.join(b'', data_out)

if actual != expect:
	print(actual)
	print(expect)
	raise AssertionError('Reactor test failed')
else:
	print('Reactor test passed')
