#version 410
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 texCoordIn;
uniform sampler2D heightField;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;

void main()
{
	float delta = 0.01;
	float scale = 0.1;
	float height = texture(heightField, texCoordIn).r * scale;
	float du = (texture(heightField, texCoordIn + vec2(delta, 0)).r * scale - height) / delta;
	float dv = (texture(heightField, texCoordIn + vec2(0, delta)).r * scale - height) / delta;
	vec3 mappedPos = vec3(position.x, height, position.y);
	vec3 normalIn = normalize(vec3(du, 1, -dv));

	gl_Position = modelViewProjectionMatrix * vec4(mappedPos, 1.0);
	// texCoord = 1.0 - texCoordIn;
	texCoord = texCoordIn;
	viewSpaceNormal = (normalMatrix * vec4(normalIn, 0.0)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(mappedPos, 1.0)).xyz;
}
