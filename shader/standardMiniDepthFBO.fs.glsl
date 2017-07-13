#version 330

in vec3 color; 
  
layout(location = 0)  out vec4 out0; // color 

uniform sampler2D tex;

in vec2 tc;

void main() 
{ 
	float col = texture2D(tex, tc).r;
	out0 = vec4(col, col, col, 1);
}
