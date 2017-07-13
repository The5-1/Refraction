#version 330

in vec3 color; 
  
layout(location = 0)  out vec4 out0; // color 


void main() 
{ 
	out0 = vec4(color, 1);
}
