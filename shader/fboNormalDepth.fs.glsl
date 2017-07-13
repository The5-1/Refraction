#version 330
  
layout(location = 0) out vec4 outNormal;

in vec3 normal;

void main() 
{ 
	outNormal = vec4(normalize(normal), 1);
}
