#version 410
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 texCoordIn;
uniform sampler2D heighField;

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
	float height = texture(heighField, texCoord).x * 0.1;
	vec3 mappedPos = vec3(position.x, height, position.y);
	vec3 normal = vec3(0, 1, 0);

	gl_Position = modelViewProjectionMatrix * vec4(mappedPos, 1.0);
	texCoord = texCoordIn;
	viewSpaceNormal = (normalMatrix * vec4(normal, 0.0)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(mappedPos, 1.0)).xyz;
}
