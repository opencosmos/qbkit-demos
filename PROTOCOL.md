# Bridge protocol

The bridge binds two sockets, a publisher and a subscriber.

Services connect to both of those sockets, with a subscriber and a publisher (respectively).

This allows the bridge to act as a bi-directional tunnel for multiple services.

Each ZeroMQ message part is framed with a (prepended) status byte before being transmitted on the serial link.

The bridge at the other end removes this stauts byte when deframing, and applies the appropriate decoding to the data as specified by the status byte, before passing it to the services.

The serial link uses KISS encoding for packetisation/depacketisation.

The status byte currently has the following flags defined:

 * `FLAG_MORE` (0x01): If this part is not the last part of a message (`ZMQ_SNDMORE`/`ZMQ_RCVMORE`).

In future, other bits of the status byte might be used to indicate other things, for example:

 * Message is compressed (other framing specifies codec).

 * Message is encrypted (other framing specifies cipher).

 * Message has checksum or FEC attached (other framing specifies what).

# ZeroMQ protocol

Each message is sent in several parts:

1: Destination label
2: Sender label
3: Session label
4: Command label
5...N: Message parts

Each "label" consists of UTF-8 text terminated by a NULL character.

The NULL character enables exact matching of subscribers (provided no label text contains NULL characters).

They also provide a crude form of validation for rejecting messages sent with other protocols.

If a receiver finds that one label is not terminated correctly, it should ignore the rest of the message.

Some implementations may only support one message part (i.e. N == 5).
