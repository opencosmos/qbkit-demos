''' Unit-test for JSON serialiser/deserialiser '''

from Json import JsonSerialiser, JsonDeserialiser
from Buffer import Buffer

actual = Buffer()
decoder = JsonDeserialiser(actual)
encoder = JsonSerialiser(decoder)
expect = {'key': 'value', 'array': [1, 2, 3, None]}

encoder.accept(expect)

if actual.data != [expect]:
	print(actual.data)
	print([expect])
	raise AssertionError('JSON round-trip test failed')
else:
	print('JSON round-trip test passed')
