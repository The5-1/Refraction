#version 330
  
layout(location = 0) in  vec3 vPosition; 
layout(location = 1) in  vec3 vNormal; 

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 normalMatrix;

uniform vec3 cameraPosition;

uniform vec3 lightPosition;
uniform int lightType;


out vec3 Normal;
out vec3 View;
out vec3 Light;
out vec3 Halfway;

void main() {
	
	vec4 posWorld =  modelMatrix * vec4(vPosition, 1);

	Normal = normalize(vec4(normalMatrix*vec4(vNormal,0.0)).xyz);
	View = normalize(cameraPosition - posWorld.xyz);
	Light = normalize(lightPosition);

	if(lightType == 0)
	{
		Light = normalize(lightPosition - posWorld.xyz); 
	}

	Halfway = normalize(vec3(Light + View));

	gl_Position = projMatrix * viewMatrix * posWorld;
}

