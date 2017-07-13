#version 330
  
layout(location = 0) in  vec3 vPosition; 
layout(location = 1) in  vec3 vNormal; 

uniform mat4 modelMatrix;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;

out vec3 normal;

void main() {
	//Without gl-Position both textures will be empty. Without this the fragment shader does not
	//Know where the triangles are
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vPosition, 1);

	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
	normal = normalMatrix * vNormal.xyz;
}
