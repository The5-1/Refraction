#version 330
  
layout(location = 0) in  vec3 vPosition; 
layout(location = 1) in  vec3 vNormal; 

const vec3 center = vec3(0,0,0);

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec4 c;

void main() {
	c = viewMatrix * modelMatrix * vec4(center, 1.0f);
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vPosition, 1.0f);
}
