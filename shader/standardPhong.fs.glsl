#version 330
precision highp float;

uniform vec3 albedoColor;
uniform vec3 ambientColor;

uniform vec3 lightColor;
uniform float specular;
uniform float glossiness;
uniform float metalness;

layout(location = 0)  out vec4 out0; // color 


in vec3 Normal;
in vec3 View;
in vec3 Light;
in vec3 Halfway;

float Phong_diffuse(vec3 n, vec3 l)
{
	return max(0.0,dot(n,l));
}


float Phong_Specular(vec3 n, vec3 h)
{
	return pow(max(0.0, dot(n,h)), glossiness) * specular;
}

void main() 
{ 
	//re-normalize after interpolation, else you got extremely visible triange artifacts!!!
	vec3 N = normalize(Normal);
	vec3 L = normalize(Light);
	vec3 V = normalize(View);
	vec3 H = normalize(Halfway);

	vec3 specColor = mix(vec3(1.0),albedoColor,metalness);

	out0 = vec4(albedoColor*(ambientColor + Phong_diffuse(N,L)*lightColor) + Phong_Specular(N,H)*specColor*lightColor, 1);
	//out0 = vec4(Phong_Specular(Normal,Halfway)* lightColor,1.0);
}
