
#include <stdafx.hpp>
#include <format_rtti.hpp>
#include <format_type_traits.hpp>
#include <type_traits>

using namespace ste::gl;

namespace ste::gl::_detail {

template <format Format, typename = std::void_t<>>
struct format_rtti_gli_format {
	static constexpr gli::format value = gli::format::FORMAT_UNDEFINED;
};
template <format Format>
struct format_rtti_gli_format<Format, std::void_t<decltype(typename format_traits<Format>::gli_format)>> {
	static constexpr gli::format value = format_traits<Format>::gli_format;
};
template <format Format>
static constexpr auto format_rtti_gli_format_v = format_rtti_gli_format<Format>::value;

template <format Format>
constexpr format_rtti format_rtti_for_format() {
	format_rtti ret;
	ret.elements = format_traits<Format>::elements;
	ret.texel_bytes = format_traits<Format>::texel_bytes;
	ret.is_depth = format_traits<Format>::is_depth;
	ret.is_float = format_traits<Format>::is_float;
	ret.is_signed = format_traits<Format>::is_signed;
	ret.is_srgb = format_traits<Format>::is_srgb;
	ret.is_normalized_integer = format_traits<Format>::is_normalized_integer;
	ret.is_scaled_integer = format_traits<Format>::is_scaled_integer;
	ret.gli_format = format_rtti_gli_format_v<Format>;

	return ret;
}

}

std::unordered_map<format, format_rtti> _internal::format_rtti_database::database =
{
	{
		format::r4g4_unorm_pack8,
		_detail::format_rtti_for_format<format::r4g4_unorm_pack8>()
	},
	{
		format::r4g4b4a4_unorm_pack16,
		_detail::format_rtti_for_format<format::r4g4b4a4_unorm_pack16>()
	},
	{
		format::b4g4r4a4_unorm_pack16,
		_detail::format_rtti_for_format<format::b4g4r4a4_unorm_pack16>()
	},
	{
		format::r5g6b5_unorm_pack16,
		_detail::format_rtti_for_format<format::r5g6b5_unorm_pack16>()
	},
	{
		format::b5g6r5_unorm_pack16,
		_detail::format_rtti_for_format<format::b5g6r5_unorm_pack16>()
	},
	{
		format::r5g5b5a1_unorm_pack16,
		_detail::format_rtti_for_format<format::r5g5b5a1_unorm_pack16>()
	},
	{
		format::b5g5r5a1_unorm_pack16,
		_detail::format_rtti_for_format<format::b5g5r5a1_unorm_pack16>()
	},
	{
		format::a1r5g5b5_unorm_pack16,
		_detail::format_rtti_for_format<format::a1r5g5b5_unorm_pack16>()
	},
	{
		format::r8_unorm,
		_detail::format_rtti_for_format<format::r8_unorm>()
	},
	{
		format::r8_snorm,
		_detail::format_rtti_for_format<format::r8_snorm>()
	},
	{
		format::r8_uscaled,
		_detail::format_rtti_for_format<format::r8_uscaled>()
	},
	{
		format::r8_sscaled,
		_detail::format_rtti_for_format<format::r8_sscaled>()
	},
	{ 
		format::r8_uint,
		_detail::format_rtti_for_format<format::r8_uint>()
	},
	{ 
		format::r8_sint,
		_detail::format_rtti_for_format<format::r8_sint>()
	},
	{
		format::r8_srgb,
		_detail::format_rtti_for_format<format::r8_srgb>()
	},
	{
		format::r8g8_unorm,
		_detail::format_rtti_for_format<format::r8g8_unorm>()
	},
	{
		format::r8g8_snorm,
		_detail::format_rtti_for_format<format::r8g8_snorm>()
	},
	{
		format::r8g8_uscaled,
		_detail::format_rtti_for_format<format::r8g8_uscaled>()
	},
	{
		format::r8g8_sscaled,
		_detail::format_rtti_for_format<format::r8g8_sscaled>()
	},
	{
		format::r8g8_uint,
		_detail::format_rtti_for_format<format::r8g8_uint>()
	},
	{
		format::r8g8_sint,
		_detail::format_rtti_for_format<format::r8g8_sint>()
	},
	{
		format::r8g8_srgb,
		_detail::format_rtti_for_format<format::r8g8_srgb>()
	},
	{
		format::r8g8b8_unorm,
		_detail::format_rtti_for_format<format::r8g8b8_unorm>()
	},
	{
		format::r8g8b8_snorm,
		_detail::format_rtti_for_format<format::r8g8b8_snorm>()
	},
	{
		format::r8g8b8_uscaled,
		_detail::format_rtti_for_format<format::r8g8b8_uscaled>()
	},
	{
		format::r8g8b8_sscaled,
		_detail::format_rtti_for_format<format::r8g8b8_sscaled>()
	},
	{
		format::r8g8b8_uint,
		_detail::format_rtti_for_format<format::r8g8b8_uint>()
	},
	{
		format::r8g8b8_sint,
		_detail::format_rtti_for_format<format::r8g8b8_sint>()
	},
	{
		format::r8g8b8_srgb,
		_detail::format_rtti_for_format<format::r8g8b8_srgb>()
	},
	{
		format::b8g8r8_unorm,
		_detail::format_rtti_for_format<format::b8g8r8_unorm>()
	},
	{
		format::b8g8r8_snorm,
		_detail::format_rtti_for_format<format::b8g8r8_snorm>()
	},
	{
		format::b8g8r8_uscaled,
		_detail::format_rtti_for_format<format::b8g8r8_uscaled>()
	},
	{
		format::b8g8r8_sscaled,
		_detail::format_rtti_for_format<format::b8g8r8_sscaled>()
	},
	{
		format::b8g8r8_uint,
		_detail::format_rtti_for_format<format::b8g8r8_uint>()
	},
	{
		format::b8g8r8_sint,
		_detail::format_rtti_for_format<format::b8g8r8_sint>()
	},
	{
		format::b8g8r8_srgb,
		_detail::format_rtti_for_format<format::b8g8r8_srgb>()
	},
	{
		format::r8g8b8a8_unorm,
		_detail::format_rtti_for_format<format::r8g8b8a8_unorm>()
	},
	{
		format::r8g8b8a8_snorm,
		_detail::format_rtti_for_format<format::r8g8b8a8_snorm>()
	},
	{
		format::r8g8b8a8_uscaled,
		_detail::format_rtti_for_format<format::r8g8b8a8_uscaled>()
	},
	{
		format::r8g8b8a8_sscaled,
		_detail::format_rtti_for_format<format::r8g8b8a8_sscaled>()
	},
	{
		format::r8g8b8a8_uint,
		_detail::format_rtti_for_format<format::r8g8b8a8_uint>()
	},
	{
		format::r8g8b8a8_sint,
		_detail::format_rtti_for_format<format::r8g8b8a8_sint>()
	},
	{
		format::r8g8b8a8_srgb,
		_detail::format_rtti_for_format<format::r8g8b8a8_srgb>()
	},
	{
		format::b8g8r8a8_unorm,
		_detail::format_rtti_for_format<format::b8g8r8a8_unorm>()
	},
	{
		format::b8g8r8a8_snorm,
		_detail::format_rtti_for_format<format::b8g8r8a8_snorm>()
	},
	{
		format::b8g8r8a8_uscaled,
		_detail::format_rtti_for_format<format::b8g8r8a8_uscaled>()
	},
	{
		format::b8g8r8a8_sscaled,
		_detail::format_rtti_for_format<format::b8g8r8a8_sscaled>()
	},
	{
		format::b8g8r8a8_uint,
		_detail::format_rtti_for_format<format::b8g8r8a8_uint>()
	},
	{
		format::b8g8r8a8_sint,
		_detail::format_rtti_for_format<format::b8g8r8a8_sint>()
	},
	{
		format::b8g8r8a8_srgb,
		_detail::format_rtti_for_format<format::b8g8r8a8_srgb>()
	},
	{
		format::a8b8g8r8_unorm_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_unorm_pack32>()
	},
	{
		format::a8b8g8r8_snorm_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_snorm_pack32>()
	},
	{
		format::a8b8g8r8_uscaled_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_uscaled_pack32>()
	},
	{
		format::a8b8g8r8_sscaled_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_sscaled_pack32>()
	},
	{
		format::a8b8g8r8_uint_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_uint_pack32>()
	},
	{
		format::a8b8g8r8_sint_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_sint_pack32>()
	},
	{
		format::a8b8g8r8_srgb_pack32,
		_detail::format_rtti_for_format<format::a8b8g8r8_srgb_pack32>()
	},
	{
		format::a2r10g10b10_unorm_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_unorm_pack32>()
	},
	{
		format::a2r10g10b10_snorm_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_snorm_pack32>()
	},
	{
		format::a2r10g10b10_uscaled_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_uscaled_pack32>()
	},
	{
		format::a2r10g10b10_sscaled_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_sscaled_pack32>()
	},
	{
		format::a2r10g10b10_uint_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_uint_pack32>()
	},
	{
		format::a2r10g10b10_sint_pack32,
		_detail::format_rtti_for_format<format::a2r10g10b10_sint_pack32>()
	},
	{
		format::a2b10g10r10_unorm_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_unorm_pack32>()
	},
	{
		format::a2b10g10r10_snorm_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_snorm_pack32>()
	},
	{
		format::a2b10g10r10_uscaled_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_uscaled_pack32>()
	},
	{
		format::a2b10g10r10_sscaled_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_sscaled_pack32>()
	},
	{
		format::a2b10g10r10_uint_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_uint_pack32>()
	},
	{
		format::a2b10g10r10_sint_pack32,
		_detail::format_rtti_for_format<format::a2b10g10r10_sint_pack32>()
	},
	{
		format::r16_unorm,
		_detail::format_rtti_for_format<format::r16_unorm>()
	},
	{
		format::r16_snorm,
		_detail::format_rtti_for_format<format::r16_snorm>()
	},
	{
		format::r16_uscaled,
		_detail::format_rtti_for_format<format::r16_uscaled>()
	},
	{
		format::r16_sscaled,
		_detail::format_rtti_for_format<format::r16_sscaled>()
	},
	{
		format::r16_uint,
		_detail::format_rtti_for_format<format::r16_uint>()
	},
	{
		format::r16_sint,
		_detail::format_rtti_for_format<format::r16_sint>()
	},
	{
		format::r16_sfloat,
		_detail::format_rtti_for_format<format::r16_sfloat>()
	},
	{
		format::r16g16_unorm,
		_detail::format_rtti_for_format<format::r16g16_unorm>()
	},
	{
		format::r16g16_snorm,
		_detail::format_rtti_for_format<format::r16g16_snorm>()
	},
	{
		format::r16g16_uscaled,
		_detail::format_rtti_for_format<format::r16g16_uscaled>()
	},
	{
		format::r16g16_sscaled,
		_detail::format_rtti_for_format<format::r16g16_sscaled>()
	},
	{
		format::r16g16_uint,
		_detail::format_rtti_for_format<format::r16g16_uint>()
	},
	{
		format::r16g16_sint,
		_detail::format_rtti_for_format<format::r16g16_sint>()
	},
	{
		format::r16g16_sfloat,
		_detail::format_rtti_for_format<format::r16g16_sfloat>()
	},
	{
		format::r16g16b16_unorm,
		_detail::format_rtti_for_format<format::r16g16b16_unorm>()
	},
	{
		format::r16g16b16_snorm,
		_detail::format_rtti_for_format<format::r16g16b16_snorm>()
	},
	{
		format::r16g16b16_uscaled,
		_detail::format_rtti_for_format<format::r16g16b16_uscaled>()
	},
	{
		format::r16g16b16_sscaled,
		_detail::format_rtti_for_format<format::r16g16b16_sscaled>()
	},
	{
		format::r16g16b16_uint,
		_detail::format_rtti_for_format<format::r16g16b16_uint>()
	},
	{
		format::r16g16b16_sint,
		_detail::format_rtti_for_format<format::r16g16b16_sint>()
	},
	{
		format::r16g16b16_sfloat,
		_detail::format_rtti_for_format<format::r16g16b16_sfloat>()
	},
	{
		format::r16g16b16a16_unorm,
		_detail::format_rtti_for_format<format::r16g16b16a16_unorm>()
	},
	{
		format::r16g16b16a16_snorm,
		_detail::format_rtti_for_format<format::r16g16b16a16_snorm>()
	},
	{
		format::r16g16b16a16_uscaled,
		_detail::format_rtti_for_format<format::r16g16b16a16_uscaled>()
	},
	{
		format::r16g16b16a16_sscaled,
		_detail::format_rtti_for_format<format::r16g16b16a16_sscaled>()
	},
	{
		format::r16g16b16a16_uint,
		_detail::format_rtti_for_format<format::r16g16b16a16_uint>()
	},
	{
		format::r16g16b16a16_sint,
		_detail::format_rtti_for_format<format::r16g16b16a16_sint>()
	},
	{
		format::r16g16b16a16_sfloat,
		_detail::format_rtti_for_format<format::r16g16b16a16_sfloat>()
	},
	{
		format::r32_uint,
		_detail::format_rtti_for_format<format::r32_uint>()
	},
	{
		format::r32_sint,
		_detail::format_rtti_for_format<format::r32_sint>()
	},
	{
		format::r32_sfloat,
		_detail::format_rtti_for_format<format::r32_sfloat>()
	},
	{
		format::r32g32_uint,
		_detail::format_rtti_for_format<format::r32g32_uint>()
	},
	{
		format::r32g32_sint,
		_detail::format_rtti_for_format<format::r32g32_sint>()
	},
	{
		format::r32g32_sfloat,
		_detail::format_rtti_for_format<format::r32g32_sfloat>()
	},
	{
		format::r32g32b32_uint,
		_detail::format_rtti_for_format<format::r32g32b32_uint>()
	},
	{
		format::r32g32b32_sint,
		_detail::format_rtti_for_format<format::r32g32b32_sint>()
	},
	{
		format::r32g32b32_sfloat,
		_detail::format_rtti_for_format<format::r32g32b32_sfloat>()
	},
	{
		format::r32g32b32a32_uint,
		_detail::format_rtti_for_format<format::r32g32b32a32_uint>()
	},
	{
		format::r32g32b32a32_sint,
		_detail::format_rtti_for_format<format::r32g32b32a32_sint>()
	},
	{
		format::r32g32b32a32_sfloat,
		_detail::format_rtti_for_format<format::r32g32b32a32_sfloat>()
	},
	{
		format::r64_uint,
		_detail::format_rtti_for_format<format::r64_uint>()
	},
	{
		format::r64_sint,
		_detail::format_rtti_for_format<format::r64_sint>()
	},
	{
		format::r64_sfloat,
		_detail::format_rtti_for_format<format::r64_sfloat>()
	},
	{
		format::r64g64_uint,
		_detail::format_rtti_for_format<format::r64g64_uint>()
	},
	{
		format::r64g64_sint,
		_detail::format_rtti_for_format<format::r64g64_sint>()
	},
	{
		format::r64g64_sfloat,
		_detail::format_rtti_for_format<format::r64g64_sfloat>()
	},
	{
		format::r64g64b64_uint,
		_detail::format_rtti_for_format<format::r64g64b64_uint>()
	},
	{
		format::r64g64b64_sint,
		_detail::format_rtti_for_format<format::r64g64b64_sint>()
	},
	{
		format::r64g64b64_sfloat,
		_detail::format_rtti_for_format<format::r64g64b64_sfloat>()
	},
	{
		format::r64g64b64a64_uint,
		_detail::format_rtti_for_format<format::r64g64b64a64_uint>()
	},
	{
		format::r64g64b64a64_sint,
		_detail::format_rtti_for_format<format::r64g64b64a64_sint>()
	},
	{
		format::r64g64b64a64_sfloat,
		_detail::format_rtti_for_format<format::r64g64b64a64_sfloat>()
	},
	{
		format::b10g11r11_ufloat_pack32,
		_detail::format_rtti_for_format<format::b10g11r11_ufloat_pack32>()
	},
	{
		format::e5b9g9r9_ufloat_pack32,
		_detail::format_rtti_for_format<format::e5b9g9r9_ufloat_pack32>()
	},
	{
		format::d16_unorm,
		_detail::format_rtti_for_format<format::d16_unorm>()
	},
	{
		format::x8_d24_unorm_pack32,
		_detail::format_rtti_for_format<format::x8_d24_unorm_pack32>()
	},
	{
		format::d32_sfloat,
		_detail::format_rtti_for_format<format::d32_sfloat>()
	}
};
