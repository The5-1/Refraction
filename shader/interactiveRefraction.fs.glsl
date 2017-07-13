#version 330
  
layout(location = 0)  out vec4 out0; // color 

in vec2 tc;


uniform sampler2D frontNormal;
uniform sampler2D frontDepth;
uniform sampler2D backNormal;
uniform sampler2D backDepth;
uniform sampler2D envDome;

uniform vec3 viewDirection;

uniform float outsideMaterial;
uniform float insideMaterial;

float angleBetweenVecs(vec3 a, vec3 b){
	return acos(dot(a, b)/(length(a) * length(b)));
}

void main() 
{ 
	out0 = vec4(color, 1);
}
