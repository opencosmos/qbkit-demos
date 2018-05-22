Known bugs:

Python implementation will not correctly encode packet beginning with escape sequence (e.g FEND FEND ... FEND FESC TFESC FEND ... FEND FEND)
