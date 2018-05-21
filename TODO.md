TODO
====

Refactor to use Python's build-in "async" sugar and concurrency, instead of explicit "select"-based reactor.

After this, incorporate ZeroMQ into client+server+bridge and deprecate the UDP versions.

This way, we can have multiple servers and/or clients binding to the same bridge, without having to manually mess around with UDP multicast groups.
