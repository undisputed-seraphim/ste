
#type frag
#version 450
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : require

layout(early_fragment_tests) in;

#include "material.glsl"
#include "gbuffer.glsl"

in v {
	vec3 frag_position;
	vec2 frag_texcoords;
	vec3 frag_normal;
	vec3 frag_tangent;
	flat int matIdx;
} vin;

uniform float height_map_scale = 1.f;

void main() {
	vec2 uv = vin.frag_texcoords;
	material_descriptor md = mat_descriptor[vin.matIdx];

	float alpha = md.alphamap.tex_handler>0 ? texture(sampler2D(md.alphamap.tex_handler), uv).x : 1.f;
	if (alpha == .0f)
		return;

	vec3 P = vin.frag_position;
	vec3 n = normalize(vin.frag_normal);
	vec3 t = normalize(vin.frag_tangent);

	normal_map(md, height_map_scale, uv, n, t, P);

	float specular = md.specular.tex_handler>0 ? texture(sampler2D(md.specular.tex_handler), uv).x : 1.f;
	vec3 diffuse = md.diffuse.tex_handler>0 ? texture(sampler2D(md.diffuse.tex_handler), uv).rgb : vec3(1.f);

	vec4 albedo = vec4(diffuse, alpha);
	uint16_t material = vin.matIdx >= 0 ? uint16_t(vin.matIdx) : material_none;

	gbuffer_store(P, albedo, specular, n, t, material, ivec2(gl_FragCoord.xy));
}
