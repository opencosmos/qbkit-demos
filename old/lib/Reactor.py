''' Basic POSIX select/poll reactor '''

from select import select
import fcntl

from Reactant import Reactant

class Binding:
	def __init__(self, reaction, events):
		''' Optional callback '''
		self.reaction = reaction
		''' Set of events to bind (Reactor.{READ,WRITE,ERROR}) '''
		self.events = events

def raise_io_error():
	raise Error("IO error state")

class Symbol:
	def __init__(self, name):
		self.name = name
	def __str__(self):
		return self.name

READ = Symbol("READ")
WRITE = Symbol("WRITE")
ERROR = Symbol("ERROR")

ERROR_REMOVE = Symbol("ERROR_REMOVE")
ERROR_IGNORE = Symbol("ERROR_IGNORE")
ERROR_THROW = Symbol("ERROR_THROW")

class Reactor(object):

	''' Dict of reactants → Binding '''
	_reactants = dict()

	''' Event handler, takes dict of reactants → set of events '''
	handler = None

	''' Function called for each reactant which is in error state '''
	on_error = None

	def __init__(self, handler=None, on_error=ERROR_THROW):
		if not callable(handler) and handler is not None:
			raise TypeError("handler is not callable")
		self.handler = handler
		if on_error == ERROR_THROW:
			on_error = raise_io_error
		elif on_error == ERROR_REMOVE:
			on_error = lambda x: self.remove(x)
		elif on_error == ERROR_IGNORE:
			on_error = lambda x: None
		elif not callable(on_error):
			raise TypeError("Invalid parameter type for on_error")
		self.on_error = on_error

	def add(self, reactant, reaction=None, events=(READ, WRITE)):
		''' Add a reactant to the reactor '''
		if not isinstance(reactant, Reactant):
			raise TypeError("Invalid paramter type: does not inherit from Reactant")
		if (reaction is None) == (self.handler is None):
			raise AssertionError('Either handler xor reaction required')
		if events in (READ, WRITE, ERROR):
			events = [events]
		events = set(events)
		if [invalid for invalid in events if not invalid in (READ, WRITE, ERROR)]:
			raise TypeError("Invalid event type")
		self._reactants[reactant] = Binding(reaction, events)

	def remove(self, reactant):
		''' Remove a reactant from the reactor '''
		del self._reactants[reactant]

	def react(self, timeout=None):
		''' Wait for a reactant to be in readable/writeable/error state, then react '''
		if timeout is None:
			timeout = 0
		r, w, e = select(
				[reactant.fileno() for reactant, binding in self._reactants.items() if READ in binding.events],
				[reactant.fileno() for reactant, binding in self._reactants.items() if WRITE in binding.events],
				[reactant.fileno() for reactant, binding in self._reactants.items() if ERROR in binding.events],
				timeout)
		for reactant in e:
			self.on_error(reactant)
		if self.handler:
			state = dict.fromkeys(self._reactants.keys())
			for reactant in self._reactants.keys():
				events = set()
				fd = reactant.fileno()
				if fd in e:
					events.add(ERROR)
					self.on_error(reactant)
				if fd in r:
					events.add(READ)
				if fd in w:
					events.add(WRITE)
				state[reactant] = events
			self.handler(state)
		else:
			for reactant, binding in dict(self._reactants).items():
				state = set()
				fd = reactant.fileno()
				if fd in r:
					state.add(READ)
				if fd in w:
					state.add(WRITE)
				if fd in e:
					state.add(ERROR)
				if state:
					binding.reaction(state)
		return r or w or e

Reactor.READ = READ
Reactor.WRITE = WRITE
Reactor.ERROR = ERROR

Reactor.ERROR_REMOVE = ERROR_REMOVE
Reactor.ERROR_IGNORE = ERROR_IGNORE
Reactor.ERROR_THROW = ERROR_THROW
