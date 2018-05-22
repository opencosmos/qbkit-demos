#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>

namespace Kiss {

struct Config
{
	static constexpr std::uint8_t FEND = 0xc0;
	static constexpr std::uint8_t FESC = 0xdb;
	static constexpr std::uint8_t TFEND = 0xdb;
	static constexpr std::uint8_t TFESC = 0xdd;
};

class Encoder
{
public:
	template <typename InputIt, typename OutputIt>
	void encode_packet(InputIt begin, InputIt end, OutputIt out)
	{
		*out++ = Config::FEND;
		for (InputIt& it = begin; it != end; ++it)
		{
			const auto byte = *it;
			if (byte == Config::FEND) {
				*out++ = Config::FESC;
				*out++ = Config::TFEND;
			} else if (byte == Config::FESC) {
				*out++ = Config::FESC;
				*out++ = Config::TFESC;
			} else {
				*out++ = byte;
			}
		}
		*out++ = Config::FEND;
	}
};

class Decoder
{
	std::size_t max_packet_length;
	std::vector<std::uint8_t> packet;
	enum State {
		idle,
		error,
		active,
		active_escape
	};
	State state = idle;

	/*
	 * Returns iterator to byte terminating the first packet decoded, or end if
	 * no complete packet was decoded.
	 *
	 * Packet data is available via "packet" member when return value indicates
	 * that a packet was decoded.
	 */
	template <typename InputIt>
	InputIt decode_packet_internal(InputIt begin, InputIt end)
	{
		InputIt it;
		for (it = begin; it != end; ++it) {
			const std::uint8_t in = *it;
			std::uint8_t out;
			/* Can we go from error state to idle */
			if (state == error) {
				if (in == Config::FEND) {
					state = idle;
				}
			}
			/* Can we go from idle state to active? */
			if (state == idle) {
				if (in != Config::FEND) {
					state = active;
					packet.clear();
				}
			}
			/* Process packet contents/terminator */
			if (state == active) {
				if (in == Config::FESC) {
					/* Escape sequence */
					state = active_escape;
				} else if (in == Config::FEND) {
					/* End of packet */
					state = idle;
					break;
				} else {
					/* Verbatim */
					out = in;
				}
			} else if (state == active_escape) {
				/* Handle escape sequences (emit byte, return to normal mode) */
				if (in == Config::TFEND) {
					state = active;
					out = Config::FEND;
				} else if (in == Config::TFESC) {
					state = active;
					out = Config::FESC;
				} else {
					/* Invalid escape sequence: transition to error state */
					state = error;
				}
			}
			/* If we're in active state, emit a byte (unless buffer overflows) */
			if (state == active) {
				if (packet.size() == max_packet_length) {
					state = error;
				} else {
					packet.push_back(out);
				}
			}
		}
		return it;
	}

public:
	Decoder(std::size_t max_packet_length) :
		max_packet_length(max_packet_length),
		packet(max_packet_length)
	{
	}
	template <typename InputIt, typename OutputIt>
	InputIt decode_packet(InputIt begin, InputIt end, OutputIt out)
	{
		InputIt it = decode_packet_internal(begin, end);
		if (it != end) {
			*out++ = packet;
			++it;
		}
		return it;
	}
	template <typename InputIt, typename OutputIt>
	void decode_all(InputIt begin, InputIt end, OutputIt out)
	{
		for (InputIt it = begin; it != end; ) {
			it = decode_packet(it, end, out);
		}
	}
};

}
