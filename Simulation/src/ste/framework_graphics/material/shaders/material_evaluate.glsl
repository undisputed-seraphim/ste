
#include "material.glsl"
#include "material_layer_unpack.glsl"

#include "subsurface_scattering.glsl"

#include "microfacet_ggx_fitting.glsl"

#include "cook_torrance.glsl"
#include "lambert_diffuse.glsl"
#include "disney_diffuse.glsl"

#include "fresnel.glsl"

#include "common.glsl"

vec3 material_evaluate_layer_radiance(material_layer_unpacked_descriptor descriptor,
									  vec3 n,
									  vec3 t,
									  vec3 b,
									  vec3 v,
									  vec3 l,
									  vec3 h,
									  float cos_critical, float sin_critical,
									  vec3 irradiance,
									  vec3 base_color,
									  vec3 diffuse_color,
									  out float D,
									  out float Gmask,
									  out float Gshadow,
									  out float F) {
	float dotLH = max(dot(l, h), .0f);
	float dotNL = max(dot(n, l), .0f);
	
	// Anisotropic roughness
	float rx = descriptor.roughness * descriptor.anisotropy_ratio;
	float ry = descriptor.roughness / descriptor.anisotropy_ratio;
	
	// Specular color
	vec3 specular_tint = vec3(1);
	vec3 c_spec = mix(specular_tint, base_color, descriptor.metallic);

	// Specular
	vec3 Specular = cook_torrance_ansi_brdf(n, t, b, 
											v, l, h,
											rx, ry,
											cos_critical, sin_critical,
											c_spec,
											D, Gmask, Gshadow, F);

	// Diffuse
	//vec3 Diffuse = diffuse_color * disney_diffuse_brdf(n, v, l, h, descriptor.roughness);
	vec3 Diffuse = diffuse_color * lambert_diffuse_brdf();

	// Evaluate BRDF
	vec3 brdf = Specular + (1.f - descriptor.metallic) * Diffuse;
	return brdf * irradiance * dotNL;
}

float material_attenuation_through_layer(float fresnel,
										 float metallic,
										 float masking) {
	float passthrough = 1.f - metallic;
	return fresnel * masking * passthrough;
}

vec3 material_beer_lambert(vec3 att, float path_length) {
	// Limit thickness to flt_min to avoid NaN when attenuation coefficient is infinite
	return exp(-max(flt_min, path_length) * att);
}

/*
 *	Evaluate radiance of material at fragment
 *
 *	@param layer		Material layer
 *	@param position		Eye space position
 *	@param n			Normal
 *	@param t			Tangent
 *	@param b			Bitangent
 *	@param v			Outbound vector (facing away from fragment to camera)
 *	@param l			Incident vector (facing away from fragment to light)
 *	@param thickness	Object thickness at shaded fragment
 *	@param ld			Light descriptor
 *	@param shadow_maps	Shadow maps
 *	@param light		Light index
 *	@param view_ray		Normalized vector from eye to position
 */
vec3 material_evaluate_radiance(material_layer_descriptor layer,
								vec3 position,
								vec3 n,
								vec3 t,
								vec3 b,
								vec3 v,
								vec3 l,
								float object_thickness,
								light_descriptor ld,
								sampler2D microfacet_refraction_fit_lut, 
								sampler2DArray microfacet_transmission_fit_lut, 
								samplerCubeArray shadow_maps, uint light,
								float light_dist,
								float occlusion,
								float external_medium_ior = 1.0002772f) {
	float D;
	float Gmask;
	float Gshadow;
	float F;
	vec3 rgb = vec3(0);
	
	material_layer_unpacked_descriptor descriptor = material_layer_unpack(layer);
	
	// We have subsurface scattering if all layers have an attenuation of less than infinity
	bool has_subsurface_scattering = object_thickness > .0f && all(lessThan(descriptor.attenuation_coefficient, vec3(inf)));

	// Early bail
	if (occlusion <= .0f && !has_subsurface_scattering)
		return vec3(.0f);
		
	// Incoming irradiance
	vec3 irradiance = light_irradiance(ld, light_dist) * occlusion;
	float top_medium_ior = external_medium_ior;

	// Attenuation at current layer
	vec3 attenuation = vec3(1.f);
	// Attenuation of outgoing radiance, used for sub-surface scattering
	vec3 sss_attenuation = vec3(1.f);

	while (true) {
		// Read layer properties
		vec3 base_color = descriptor.color.rgb;
		float thickness = material_is_base_layer(descriptor) ? object_thickness : descriptor.thickness;
		float metallic = descriptor.metallic;
		float roughness = descriptor.roughness;
		vec3 attenuation_coefficient = descriptor.attenuation_coefficient;
		float bottom_medium_ior = descriptor.ior;
		
		// Compute sine and cosine of critical angle
		float sin_critical = bottom_medium_ior / top_medium_ior;
		float cos_critical = sin_critical < 1.f ? 
								sqrt(1.f - sin_critical * sin_critical) :
								.0f;
		// Compute fresnel reflection at 0 angle incidence
		float F0 = fresnel_F0(sin_critical);

		// Evaluate total inner (downwards into material) and outer (upwards towards eye) transmission
		float inner_transmission_ratio = ggx_transmission_ratio(microfacet_transmission_fit_lut, 
																v, n, 
																roughness, 
																sin_critical);
		float outer_transmission_ratio = ggx_transmission_ratio(microfacet_transmission_fit_lut, 
																l, n, 
																roughness, 
																1.f / sin_critical);

		// Half vector
		vec3 h = normalize(v + l);

		// Evaluate refracted vectors
		vec3 refracted_v = -ggx_refract(microfacet_refraction_fit_lut,
										v, n,
										roughness,
										sin_critical);
		vec3 refracted_l = -ggx_refract(microfacet_refraction_fit_lut,
										l, n,
										roughness,
										sin_critical);
	
		// Compute total and outer path lengths inside current layer
		float dotNV = max(epsilon, dot(n,refracted_v));
		float dotNL = max(epsilon, dot(n,refracted_l));
		float path_length = thickness * (1.f / dotNV + 1.f / dotNL);
		float outer_path_length = thickness / dotNL;
		
		// Compute light extinction in layer
		vec3 extinction = material_beer_lambert(attenuation_coefficient, path_length);
		vec3 outer_extinction = material_beer_lambert(attenuation_coefficient, outer_path_length);
		// Diffuse light is scattered light inside layer, unattenuated light doesn't contribute to diffuse
		vec3 scattering = inner_transmission_ratio * (vec3(1.f) - extinction) * base_color;

		// Evaluate layer BRDF
		rgb += attenuation * material_evaluate_layer_radiance(descriptor,
															  n, t, b,
															  v, l, h,
															  cos_critical, sin_critical,
															  irradiance,
															  base_color,
															  scattering,
															  D, Gmask, Gshadow, F);
							
		// Update incident and outgoing vectors to refracted ones before continuing to next layer
		v = refracted_v;
		l = refracted_l;
	
		// Compute attenuated light due to Fresnel, microfacet masking-shadowing and metallicity
		float layer_surface_inner_attenuation = material_attenuation_through_layer(inner_transmission_ratio, 
																				   metallic,
																				   Gshadow);
		float layer_surface_outer_attenuation = material_attenuation_through_layer(outer_transmission_ratio, 
																				   metallic,
																				   Gmask);
		float total_layer_surface_attenuation = layer_surface_inner_attenuation * layer_surface_outer_attenuation;

		// Update attenuation at layer surface
		attenuation *= total_layer_surface_attenuation;
		// For sub-surface scattering we assume normal incidence of light, and we attenuated on (presumed) incident and outgoing sides
		sss_attenuation *= layer_surface_outer_attenuation * base_color * 
						   material_attenuation_through_layer(1.f - F0, metallic, 1.f) * base_color;

		// If this is the base layer, stop
		if (material_is_base_layer(descriptor))
			break;

		// Otherwise, update attenuation with attenuated (absorbed and scattered) light
		attenuation *= extinction;
		sss_attenuation *= outer_extinction * material_beer_lambert(attenuation_coefficient, thickness);

		// Update ior and descriptor for next layer
		top_medium_ior = bottom_medium_ior;
		descriptor = material_layer_unpack(mat_layer_descriptor[descriptor.next_layer_id]);
	}
	
	// Sub-surface scattering
	has_subsurface_scattering = has_subsurface_scattering && all(lessThan(descriptor.attenuation_coefficient, vec3(inf)));
	bool fully_attenuated = all(lessThan(sss_attenuation, vec3(epsilon)));

	if (has_subsurface_scattering && !fully_attenuated) {
		rgb += sss_attenuation * subsurface_scattering(descriptor,
													   position,
													   n,
							 						   object_thickness,
													   ld,
													   shadow_maps, light,
													   -v);
	}

	return rgb;
}
