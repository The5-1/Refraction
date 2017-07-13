#version 330
  
layout(location = 0)  out vec4 out0; // color 

in vec2 uvc;

uniform sampler2D envMap;

void main() 
{ 
	out0 = vec4(texture2D(envMap, uvc).rgb, 1);

}
