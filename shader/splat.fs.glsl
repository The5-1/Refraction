#version 330
  
layout(location = 0)  out vec4 out0; // color 

in vec4 c;

uniform int width;
uniform int height;

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D position;

uniform float radius;
uniform vec3 splatColor;

void main() 
{ 
	// use this to index the gbuffer textures
	vec2 tc = gl_FragCoord.xy / vec2(width, height);

	// TODO:
	// - gbuffer position in splat radius?
	// - diffuse shading
	// - distance attenuation
	
	vec3 dif = texture2D(diffuse, tc).rgb;
	vec3 nor = texture2D(normal, tc).rgb;
	vec3 pos = texture2D(position, tc).rgb;
	
	if(length(pos-c.xyz) > radius)
        discard;
    
    float lightint = 1/(length(pos-c.xyz)*length(pos-c.xyz));
    vec3 lightDir = normalize(c.xyz-pos);
    
    
    
    vec3 lightout = max(dot(nor,lightDir),0) * dif * lightint;
	

	out0 = vec4(splatColor * lightout, 1.0);
}
