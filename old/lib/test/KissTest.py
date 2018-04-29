''' Unit test for Kiss module '''

from Kiss import KissEncoder, KissDecoder, end_of_frame, invalid_frame
from Buffer import Buffer;

def dump(data):
	''' Pretty-print sequence of frames '''
	for datum in data:
		if datum == end_of_frame:
			print('')
		elif datum == invalid_frame:
			print('<INVALID FRAME>')
		else:
			print(datum, end='')

# Buffer for storing output of test pipeline
actual = Buffer()

# Build test pipeline: input | encoder | decoder | actual
decoder = KissDecoder(actual)
encoder = KissEncoder(decoder)

# Input strings
frames = [b'Hello world!', b'Bon dia!', b'\xc0', b'\xdb', b'\xdc', b'\xdd', b'\xc0\xdb\xdc\xdd\xc0']

# Push frames into pipeline and build list of expected output
expect = []
for frame in frames:
	expect += [frame]
	encoder.accept(frame)

# Create an invalid frame
expect += [invalid_frame]

encoder.open()
for c in list(b'Bonjour'):
	encoder.accept(c)
encoder.accept(invalid_frame)
encoder.close()

# Verify result
if expect == actual.data:
	print('KISS round-trip test passed')
else:
	print('Expect')
	dump(expect)
	print('')
	print('Actual')
	dump(actual.data)
	raise AssertionError('KISS round-trip test failed')
