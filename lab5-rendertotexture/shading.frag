#version 410

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform vec3 material_emission;

uniform int has_color_texture;
uniform sampler2D colorMap;
uniform int has_emission_texture;
uniform sampler2D emissiveMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
uniform sampler2D environmentMap;
uniform sampler2D irradianceMap;
uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;


vec3 calculateDirectIllumination(vec3 wo, vec3 n, vec3 base_color)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.2 - Calculate the radiance Li from the light, and the direction
	//            to the light. If the light is backfacing the triangle,
	//            return vec3(0);
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = normalize(viewSpaceLightPosition - viewSpacePosition);

	if (dot(wi, n) <= 0.0) {
		return vec3(0.0); // Light is facing back of the triangle.
	}
	// Distance between light and current point.
	float d = distance(viewSpaceLightPosition, viewSpacePosition);
	// Find the incoming light radiance.
	vec3 li = point_light_intensity_multiplier * point_light_color * 1 / (d * d);

	///////////////////////////////////////////////////////////////////////////
	// Task 1.3 - Calculate the diffuse term and return that as the result
	///////////////////////////////////////////////////////////////////////////
	vec3 diffuse_term = base_color * 1.0 / PI * abs(dot(n, wi)) * li;

	///////////////////////////////////////////////////////////////////////////
	// Task 2 - Calculate the Torrance Sparrow BRDF and return the light
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////

	vec3 wh = normalize(wi + wo);
	float fresnel_term = material_fresnel + (1.0 - material_fresnel) * pow(1.0 - dot(wh, wi), 5.0);
	// N.B. Make sure we are not taking a power of a negative by clamping.
	float microfacet_term = (material_shininess + 2.0) / (2.0 * PI) * pow(max(0.001, dot(n, wh)), material_shininess);
	float masking_term = min(1.0, min(
		2.0 * dot(n, wh) * dot(n, wo) / dot(wo, wh),
		2.0 * dot(n, wh) * dot(n, wi) / dot(wo, wh)
	));
	float brdf = fresnel_term * microfacet_term * masking_term / 4.0 / dot(n, wo) / dot(n, wi);

	///////////////////////////////////////////////////////////////////////////
	// Task 3 - Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////

	vec3 dielectric_term = brdf * dot(n, wi) * li + (1 - fresnel_term) * diffuse_term;
	vec3 metal_term = brdf * base_color * dot(n, wi) * li;
	vec3 direct_illum = material_metalness * metal_term + (1 - material_metalness) * dielectric_term;
	return direct_illum;
}

vec2 sphereLookup(vec3 dir) {
	// Stolen from background.frag
	vec4 pixel_world_pos = viewInverse * vec4(texCoord * 2.0 - 1.0, 1.0, 1.0);
	pixel_world_pos = (1.0 / pixel_world_pos.w) * pixel_world_pos;

	// Calculate the spherical coordinates of the direction
	float theta = acos(max(-1.0f, min(1.0f, dir.y)));
	float phi = atan(dir.z, dir.x);
	if (phi < 0.0f) {
		phi = phi + 2.0f * PI;
	}

	// Use these to lookup the color in the environment map
	vec2 lookup = vec2(phi / (2.0 * PI), 1 - theta / PI);
	return lookup;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n, vec3 base_color)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 5 - Lookup the irradiance from the irradiance map and calculate
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////

	// Calculate the world-space direction from the camera to that position
	vec3 dir = vec3(viewInverse * vec4(n, 0.0));
	vec2 indirect_lookup = sphereLookup(vec3(viewInverse * vec4(n, 0.0)));
	vec3 indirect_illum = environment_multiplier * texture(irradianceMap, indirect_lookup).rgb;
	vec3 diffuse_term = base_color * (1.0 / PI) * indirect_illum;

	///////////////////////////////////////////////////////////////////////////
	// Task 6 - Look up in the reflection map from the perfect specular
	//          direction and calculate the dielectric and metal terms.
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = vec3(viewInverse * vec4(reflect(-wo, n), 0.0));
	vec3 wh = normalize(wi + wo);

	vec2 reflection_lookup = sphereLookup(wi);
	// // Convert to roughness because we need a parameter that varies linearly to choose a mipmap hierarchy.
	float roughness = sqrt(sqrt(2 / (material_shininess + 2.0)));
	vec3 li = environment_multiplier * textureLod(reflectionMap, reflection_lookup, roughness * 7.0).rgb;

	float fresnel_term = material_fresnel + (1.0 - material_fresnel) * pow(1.0 - dot(wh, wi), 5.0);
	vec3 dielectric_term = fresnel_term * li + (1.0 - fresnel_term) * diffuse_term;
	vec3 metal_term  = fresnel_term * base_color * li;
	return material_metalness * metal_term + (1 - material_metalness) * dielectric_term;
}


void main()
{
	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	vec3 base_color = material_color;
	if(has_color_texture == 1)
	{
		base_color *= texture(colorMap, texCoord).rgb;
	}

	// Direct illumination
	vec3 direct_illumination_term = calculateDirectIllumination(wo, n, base_color);

	// Indirect illumination
	vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n, base_color);

	///////////////////////////////////////////////////////////////////////////
	// Add emissive term. If emissive texture exists, sample this term.
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission;
	if(has_emission_texture == 1)
	{
		emission_term *= texture(emissiveMap, texCoord).rgb;
	}

	fragmentColor.rgb = direct_illumination_term + indirect_illumination_term + emission_term;
}
