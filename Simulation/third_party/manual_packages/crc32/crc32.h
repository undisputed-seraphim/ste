// //////////////////////////////////////////////////////////
// Crc32.h
// Copyright (c) 2011-2016 Stephan Brumme. All rights reserved.
// Slicing-by-16 contributed by Bulat Ziganshin
// Tableless bytewise CRC contributed by Hagai Gold
// see http://create.stephan-brumme.com/disclaimer.html
//

#pragma once

#include <cstdint>
#include <cstddef>

namespace crc32 {

// if running on an embedded system, you might consider shrinking the
// big Crc32Lookup table by undefining these lines:
#define CRC32_USE_LOOKUP_TABLE_BYTE
#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_4
#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_8
//#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
// - crc32_bitwise  doesn't need it at all
// - crc32_halfbyte has its own small lookup table
// - crc32_1byte_tableless and crc32_1byte_tableless2 don't need it at all
// - crc32_1byte    needs only Crc32Lookup[0]
// - crc32_4bytes   needs only Crc32Lookup[0..3]
// - crc32_8bytes   needs only Crc32Lookup[0..7]
// - crc32_4x8bytes needs only Crc32Lookup[0..7]
// - crc32_16bytes  needs all of Crc32Lookup
// using the aforementioned #defines the the table is automatically fitted to your needs

// crc32_fast selects the fastest algorithm depending on flags (CRC32_USE_LOOKUP_...)
/// compute CRC32 using the fastest algorithm for large datasets on modern CPUs
extern std::uint32_t crc32_fast(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);

/// compute CRC32 (bitwise algorithm)
extern std::uint32_t crc32_bitwise(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
/// compute CRC32 (half-byte algoritm)
extern std::uint32_t crc32_halfbyte(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);

#ifdef CRC32_USE_LOOKUP_TABLE_BYTE
/// compute CRC32 (standard algorithm)
extern std::uint32_t crc32_1byte(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
#endif

/// compute CRC32 (byte algorithm) without lookup tables
extern std::uint32_t crc32_1byte_tableless(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
/// compute CRC32 (byte algorithm) without lookup tables
extern std::uint32_t crc32_1byte_tableless2(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_4
/// compute CRC32 (Slicing-by-4 algorithm)
extern std::uint32_t crc32_4bytes(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
#endif

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_8
/// compute CRC32 (Slicing-by-8 algorithm)
extern std::uint32_t crc32_8bytes(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
/// compute CRC32 (Slicing-by-8 algorithm), unroll inner loop 4 times
extern std::uint32_t crc32_4x8bytes(const void *data, std::size_t length, std::uint32_t previousCrc32 = 0);
#endif

#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
/// compute CRC32 (Slicing-by-16 algorithm)
extern std::uint32_t crc32_16bytes(const void* data, std::size_t length, std::uint32_t previousCrc32 = 0);
/// compute CRC32 (Slicing-by-16 algorithm, prefetch upcoming data blocks)
extern std::uint32_t crc32_16bytes_prefetch(const void* data, std::size_t length, std::uint32_t previousCrc32 = 0, std::size_t prefetchAhead = 256);
#endif

}
