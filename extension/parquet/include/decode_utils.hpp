#pragma once

#include "iostream"
#include "resizable_buffer.hpp"
#include "rle_bitpacking_hybrid_encoder.h"

namespace duckdb {
class ParquetDecodeUtils {

public:
	template <class T>
	static T ZigzagToInt(const T n) {
		return (n >> 1) ^ -(n & 1);
	}

	static const uint32_t BITPACK_MASKS[];
	static const uint8_t BITPACK_DLEN;

	template <typename T>
	static uint32_t BitUnpack(ByteBuffer &buffer, uint8_t &bitpack_pos, T *dest, uint32_t count, uint8_t width) {
		if (count % 8 == 0)
		{
			auto src = reinterpret_cast<uint8_t *>(buffer.ptr);
			auto destptr = reinterpret_cast<uint32_t *>(dest);
			uint32_t count1 = count / 8;
			for (uint32_t i = 0; i < count1; i++) {
				(bitpacking_unpack8_funcs[width])(src, destptr);
				src += width;
				destptr += 8;
			}
			buffer.inc(width * count1);
		}
		else
		{
			auto mask = BITPACK_MASKS[width];
			for (uint32_t i = 0; i < count; i++) {
				T val = (buffer.get<uint8_t>() >> bitpack_pos) & mask;
				bitpack_pos += width;
				while (bitpack_pos > BITPACK_DLEN) {
					buffer.inc(1);
					val |= (buffer.get<uint8_t>() << (BITPACK_DLEN - (bitpack_pos - width))) & mask;
					bitpack_pos -= BITPACK_DLEN;
				}
				dest[i] = val;
			}
		}
		return count;
	}

	template <class T>
	static T VarintDecode(ByteBuffer &buf) {
		T result = 0;
		uint8_t shift = 0;
		while (true) {
			auto byte = buf.read<uint8_t>();
			result |= (byte & 127) << shift;
			if ((byte & 128) == 0)
				break;
			shift += 7;
			if (shift > sizeof(T) * 8) {
				throw std::runtime_error("Varint-decoding found too large number");
			}
		}
		return result;
	}
};
} // namespace duckdb
