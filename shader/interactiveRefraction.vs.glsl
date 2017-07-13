#version 330
  
layout(location = 0) in  vec3 vPosition; 
layout(location = 1) in  vec3 vNormal; 

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec2 tc;

void main() {
	tc = vPosition.xy * vec2(0.5) + vec2(0.5);
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vPosition, 1);
}

