
const int LightTypeSphere = 0;
const int LightTypeDirectional = 1;

const float light_cutoff = 0.001f;

struct light_descriptor {
	vec4 position_direction;
	vec4 diffuse;

	float luminance;
	float radius;
	float effective_range;

	int type;
};

layout(std430, binding = light_buffers_first) buffer light_data {
	light_descriptor light_buffer[];
};

layout(std430, binding = light_buffers_first + 1) buffer light_transform_data {
	vec4 light_transform_buffer[];
};

vec3 light_incidant_ray(light_descriptor ld, int i, vec3 position) {
	if (ld.type == LightTypeDirectional) return -light_transform_buffer[i].xyz;
	else return light_transform_buffer[i].xyz - position;
}

float light_attenuation_factor(light_descriptor ld, float dist) {
	if (ld.type == LightTypeDirectional)
		return 1;
	else {
		float a = max(.001f, dist / ld.radius);
		return a*a;
	}
}

float light_effective_range(light_descriptor ld) {
	if (ld.type == LightTypeDirectional)
		return +1.0f / .0f;		// Infinity
	else
		return ld.effective_range;
}

void light_transform(mat4 mv, mat3 rmv) {
	for (int i = 0; i < light_buffer.length(); ++i) {
		if (light_buffer[i].type == LightTypeSphere)
			light_transform_buffer[i] = mv * vec4(light_buffer[i].position_direction.xyz, 1);
		else
			light_transform_buffer[i].xyz = rmv * light_buffer[i].position_direction.xyz;
	}
}
