#version 330
  
layout(location = 0) in  vec3 vPosition; 
layout(location = 1) in  vec3 vNormal; 

uniform vec2 downLeft;
uniform vec2 upRight;

out vec2 tc;

void main() {
	
	vec2 pos[4];
	vec2 uvs[4];
	
	pos[0] = downLeft;
	pos[1] = vec2(upRight.x, downLeft.y);
	pos[3] = vec2(downLeft.x, upRight.y);
	pos[2] = upRight;

	uvs[0] = vec2(0,  0);
	uvs[1] = vec2(1,  0);
	uvs[3] = vec2(0,  1);
	uvs[2] = vec2(1,  1);

	tc = uvs[gl_VertexID];
	gl_Position = vec4((pos[gl_VertexID]*2.0)-vec2(1.0), 0, 1.0);
}

