''' Unit-test for Client & Server (mini-integration test really) '''

from Server import Server
from Client import Client

from Functional import Consumer

server = Server(Consumer(), lambda x: x)
client = Client(Consumer(), poll=lambda:None)

server.backend = client
client.backend = server

for x in range(1, 10):
	if client.request('Potato') != 'Potato':
		raise AssertionError('Client/Server echo test failed')

server = Server(Consumer(), lambda x: x * x)
client = Client(Consumer(), poll=lambda:None)

# Link client and server directly (typically, they would be linked via some network interface)
server.backend = client
client.backend = server

for x in range(1, 10):
	if client.request(x) != x * x:
		raise AssertionError('Client/Server square test failed')

print('Client/Server test passed')
