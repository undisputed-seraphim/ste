
#include "light_type.glsl"

#include "common.glsl"
#include "pack.glsl"
#include "chromaticity.glsl"

#include "quaternion.glsl"
#include "dual_quaternion.glsl"

#include "girenderer_transform_buffer.glsl"

#include "light_transport.glsl"
#include "atmospherics.glsl"

const int max_active_lights_per_frame = 24;
const int max_active_directional_lights_per_frame = 4;
const int total_max_active_lights_per_frame = max_active_lights_per_frame + max_active_directional_lights_per_frame;

struct light_descriptor {
	// position: Light position for spherical, direction for directional lights.
	// Radius is light's radius (all light types)
	// Emittance is the emitted luminance (all light types)
	vec3 position;	float radius;
	vec3 emittance;	uint type;
	// Light effective range
	float effective_range;
	// Normal
	uint normal_pack;
	// Surface area
	float sqrt_surface_area;
	// polygonal_light_points_and_offset specifies the number of points and offset into the buffer (polygonal lights only)
	uint polygonal_light_points_and_offset;
	
	// Texture
	layout(bindless_sampler) sampler2D texture;
	
	// directional_distance: For directional lights only, distance from origin opposite to lights direction (i.e. -directional_distance*position)
	float directional_distance;
	uint cascade_idx;
	
	//! The rest is used internally only
	
	// transformed_position: Light position in eye space for spherical, direction in eye space for directional lights.
	vec3 transformed_position;

	// shadow_face_mask: For masking (culling) shadow map cube faces (for spherical) or cascades (for directional).
	uint shadow_face_mask;
};

/*
 *	Transforms light's position/direction based on transformation dual quaternion
 */
vec3 light_transform(dual_quaternion transform, light_descriptor ld) {
	return light_type_is_directional(ld.type) ?
				quat_mul_vec(transform.real, ld.position) :
				dquat_mul_vec(transform, ld.position);
}

/*
 *	Calculates the incident ray in eye-space for sampling position
 */
vec3 light_incidant_ray(light_descriptor ld, vec3 position) {
	if (light_type_is_directional(ld.type)) return -ld.transformed_position;
	else return ld.transformed_position - position;
}

/*
 *	Calculate light attenuation at specified distance. Applicable for virtual lights, approximates other lights.
 *
 *	@param ld			Light descriptor.
 *	@param dist			Distance.
 */
float virtual_light_attenuation(light_descriptor ld, float dist) {
	float a = max(.0f, dist / ld.radius - 1.f);
	return 1.f / (1.f + a*a);
}

/*
 *	Calculate light attenuation at specified distance. Applicable for spherical/directional, polygonal and quad lights.
 *
 *	@param ld			Light descriptor.
 *	@param dist			Distance.
 *	@param wl			Point to light vector, world coordinates.
 */
float shaped_light_attenuation(light_descriptor ld, float dist, vec3 wl) {
	bool two_sided = light_type_is_two_sided(ld.type);
	bool sphere = !light_type_is_shaped(ld.type) || light_shape_is_sphere(ld.type);
	vec3 n = snorm2x32_to_norm3x32(unpackSnorm2x16(ld.normal_pack));
	float sqrtA = ld.sqrt_surface_area;

	float d = sphere ? 
					1.f : 
					-dot(wl, n);
	if (two_sided)
		d = abs(d);
	
	return max(.0f, d) * sqr(sqrtA / dist);
}

/*
 *	Returns the light irradiance illuminating from light source at 0 distance. 
 *
 *	@param ld			Light descriptor.
 */
vec3 irradiance(light_descriptor ld) {
	return ld.emittance;
}

/*
 *	Get polygonal light points count
 *
 *	@param ld			Light descriptor.
 */
uint light_get_polygon_point_counts(light_descriptor ld) {
	return ld.polygonal_light_points_and_offset >> 24;
}

/*
 *	Get polygonal light offset into points buffer
 *
 *	@param ld			Light descriptor.
 */
uint light_get_polygon_point_offset(light_descriptor ld) {
	return ld.polygonal_light_points_and_offset & 0x00FFFFFF;
}
