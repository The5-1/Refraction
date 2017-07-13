#include <iostream>

#include <Ant/AntTweakBar.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "helper.h"

#define M_PI 3.14159265359

// globals
float rotationAngle = 0.0f;
float degreesPerSecond = 1.f;
bool autoRotate = true;
float radius = 8.0f;

cameraSystem cam(1.0f, 1.0f, glm::vec3(3.0f, 16.0f, 22.f));
Timer timer;

//Environment Map
Image *image = 0;
int envMap = 0;

// render objects
simpleQuad * quad = 0;
simpleModel *teaPot = 0;
groundPlane *plane = 0;
solidSphere *sphere = 0;

// shaders
GLuint quadShader;
GLuint gbufferShader;
GLuint stencilShader;
GLuint splatShader;
GLuint stencilDebugShader;
GLuint standardShader;
GLuint standardMiniColorFBOShader;
GLuint standardMiniDepthFBOShader;
GLuint fboNormalDepthShader;
GLuint mapDomeShader;
GLuint standardPhongShader;

//Camera
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 normalMatrix;
glm::mat4 thisModelMatrix;

glm::vec3 lightDir = glm::vec3(-0.13f, 0.98f, -0.17f);
glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);

// frame buffer object
Fbo *fbo = 0;
Fbo *fboNormalDepth = 0;
Fbo *fboNormalDepth2 = 0;

// textures
Tex *diffuse = 0, *normal = 0, *position = 0, *depth = 0;
Tex *fboND_normal = 0, *fboND_depth = 0;
Tex *fboND_normal2 = 0, *fboND_depth2 = 0;

// tweak bar
TwBar *tweakBar;

bool debugShading = false;
bool stencilCulling = false;
bool debugStencil = false;
bool clearDebugStencil = false;
bool splatting = false;

int currentScene = 2;

void setupTweakBar() {
	TwInit(TW_OPENGL_CORE, NULL);
	tweakBar = TwNewBar("Settings");
	TwAddVarRW(tweakBar, "CurrentScene", TW_TYPE_INT32, &currentScene, "label='Current Scene'");

	TwAddSeparator(tweakBar, "sep", nullptr);
	TwAddVarRW(tweakBar, "lightDirection", TW_TYPE_DIR3F, &lightDir, "label='Light Direction'");
	TwAddVarRW(tweakBar, "Rotate", TW_TYPE_BOOLCPP, &autoRotate, " label='Auto Rotation' ");
	TwAddVarRW(tweakBar, "RotSpeed", TW_TYPE_FLOAT, &degreesPerSecond, " label='Rotation Speed' min=0 step=0.1 max=360 ");

	TwAddSeparator(tweakBar, "sep0", nullptr);
	TwAddVarRW(tweakBar, "DebugShader", TW_TYPE_BOOLCPP, &debugShading, " label='Debug Shader' ");

	TwAddSeparator(tweakBar, "sep1", nullptr);
	TwAddVarRW(tweakBar, "StencilCulling", TW_TYPE_BOOLCPP, &stencilCulling, " label='Stencil Culling' ");
	TwAddVarRW(tweakBar, "StencilDebug", TW_TYPE_BOOLCPP, &debugStencil, " label='Debug' ");
	TwAddVarRW(tweakBar, "StencilDebugClear", TW_TYPE_BOOLCPP, &clearDebugStencil, " label='Clear' ");

	TwAddSeparator(tweakBar, "sep2", nullptr);
	TwAddVarRW(tweakBar, "Splatting", TW_TYPE_BOOLCPP, &splatting, " label='Splatting' ");
	TwAddVarRW(tweakBar, "SplatSize", TW_TYPE_FLOAT, &radius, " label='Splat Size' min=1 max=10 step=0.5");

}



void init() {
	glEnable(GL_TEXTURE_2D);
	fbo = new Fbo("DR", WIDTH, HEIGHT, 3);
	gl_check_error("fbo");
	
	diffuse = new Tex(WIDTH, HEIGHT, GL_RGBA32F, GL_RGBA, GL_FLOAT);	gl_check_error("diffuse tex");
	normal = new Tex(WIDTH, HEIGHT, GL_RGBA32F, GL_RGBA, GL_FLOAT);	gl_check_error("normal tex");
	position = new Tex(WIDTH, HEIGHT, GL_RGBA32F, GL_RGBA, GL_FLOAT);	gl_check_error("position tex");
	depth = new Tex(WIDTH, HEIGHT, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);
	gl_check_error("depth tex");

	fbo->Bind();
	//Why?
	//glActiveTexture(GL_TEXTURE0);
	//Why?
	//diffuse->Bind();

	fbo->AddTextureAsColorbuffer("diffuse", diffuse);
	fbo->AddTextureAsColorbuffer("normal", normal);
	fbo->AddTextureAsColorbuffer("position", position);
	fbo->AddTextureAsDepthbuffer(depth);
	fbo->Check();
	fbo->Unbind();
	gl_check_error("post fbo setup");

	//FBO_ND 1
	fboNormalDepth = new Fbo("fboND", WIDTH, HEIGHT, 1);
	gl_check_error("fboND");
	fboND_normal = new Tex(WIDTH, HEIGHT, GL_RGBA32F, GL_RGBA, GL_FLOAT);	gl_check_error("normal tex");
	fboND_depth = new Tex(WIDTH, HEIGHT, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT); gl_check_error("depth tex");
	fboNormalDepth->Bind();
	fboNormalDepth->AddTextureAsColorbuffer("fboND_normal", fboND_normal);
	fboNormalDepth->AddTextureAsDepthbuffer(fboND_depth);
	fboNormalDepth->Check();
	fboNormalDepth->Unbind();
	gl_check_error("post fbo setup");

	//FBO_ND 1
	fboNormalDepth2 = new Fbo("fboND2", WIDTH, HEIGHT, 1);
	gl_check_error("fboND2");
	fboND_normal2 = new Tex(WIDTH, HEIGHT, GL_RGBA32F, GL_RGBA, GL_FLOAT);	gl_check_error("normal tex");
	fboND_depth2 = new Tex(WIDTH, HEIGHT, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT); gl_check_error("depth tex");
	fboNormalDepth2->Bind();
	fboNormalDepth2->AddTextureAsColorbuffer("fboND_normal", fboND_normal2);
	fboNormalDepth2->AddTextureAsDepthbuffer(fboND_depth2);
	fboNormalDepth2->Check();
	fboNormalDepth2->Unbind();
	gl_check_error("post fbo setup");

	//Create Environment map
	image = new Image("./data/waterfall.png");
	envMap = image->makeTexture();	

	// objects
	teaPot = new simpleModel("./data/teapot.obj");
	teaPot->upload();

	plane = new groundPlane(0.f, 12.f);
	plane->upload();

	quad = new simpleQuad();
	quad->upload();

	sphere = new solidSphere(1, 20, 20);
	sphere->upload();

}

void renderMap(int program) {
	gl_check_error("pre map");
	//glClearColor(0.2f, 0.2f, 0.2f, 1);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);

	uniform(program, "envMap", 0);

	uniform(program, "viewMatrix", viewMatrix);
	uniform(program, "projMatrix", projMatrix);

	float theta = M_PI;
	glm::mat4x4 rotationX = glm::mat4x4(1, 0, 0, 0,
		0, glm::cos(theta), -glm::sin(theta), 0,
		0, glm::sin(theta), glm::cos(theta), 0,
		0, 0, 0, 1);

	glm::mat4x4 rotationY = glm::mat4x4(glm::cos(theta), 0, glm::sin(theta), 0,
		0, 1, 0, 0,
		-glm::sin(theta), 0, glm::cos(theta), 0,
		0, 0, 0, 1);

	glm::mat4x4 rotationZ = glm::mat4x4(glm::cos(theta), -glm::sin(theta), 0, 0,
		glm::sin(theta), glm::cos(theta), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	uniform(program, "modelMatrix", rotationX * glm::scale(glm::vec3(20.0f, 20.0f, 20.0f)));

	sphere->draw();

	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(0);
	gl_check_error("post map");
}

void renderPot(GLuint program) {
	gl_check_error("pre pot");
	glUseProgram(program);

	uniform(program, "envMap", 0);

	uniform(program, "viewMatrix", viewMatrix);
	uniform(program, "projMatrix", projMatrix);
	
	uniform(program, "modelMatrix", glm::translate(glm::vec3(0, -0.33, 0)));

	teaPot->draw();

	glUseProgram(0);
	gl_check_error("post pot");
}

void renderScene(const int program) {

	glUseProgram(program);

	uniform(program, "color", glm::vec3(0.8f, 0.8f, 0.8f));
	uniform(program, "viewMatrix", viewMatrix);
	uniform(program, "projMatrix", projMatrix);

	// render pots
	for (int i = 0; i < 10; ++i) {
		float rad = ((float)i / 10.f) * M_PI * 2;
		float dist = 8.f;
		float x = cos(rad)*dist;
		float y = sin(rad)*dist;
		uniform(program, "modelMatrix", glm::translate(glm::vec3(x, 0, y)) * glm::scale(glm::mat4(), glm::vec3(2.5, 2.5, 2.5)));
		teaPot->draw();
	}

	// render ground plane
	uniform(program, "modelMatrix", glm::mat4());
	plane->draw();

	glUseProgram(0);
}

float splatDistFromOrigin() {
	return (cos(rotationAngle*0.7) + 1.0f) * 4.f + 1.0f;
}


void drawSplats(const int program, const float radius, const bool asLight) {

	glUseProgram(program);
	uniform(program, "viewMatrix", viewMatrix);
	uniform(program, "projMatrix", projMatrix);


	glm::vec3 splatColors[6] = { glm::vec3(1, 0, 0),
		glm::vec3(1, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(1, 0, 1) };

	for (int i = 0; i < 6; ++i) {
		float rad = ((float)i / 6.f) * M_PI * 2 + rotationAngle;
		float dist = splatDistFromOrigin();
		float x = cos(rad)*dist;
		float y = sin(rad)*dist;
		if (asLight) {
			uniform(program, "radius", radius);
			uniform(program, "splatColor", splatColors[i]);
		}
		else
			uniform(program, "color", splatColors[i]);
		uniform(program, "modelMatrix", glm::translate(glm::vec3(x, 2.8, y)) * glm::scale(glm::mat4(), glm::vec3(radius, radius, radius)));
		sphere->draw();
	}

	glUseProgram(0);
}

void display() {

	glClearColor(0.2f, 0.2f, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	timer.update();
	if (autoRotate)
		rotationAngle += timer.intervall*degreesPerSecond;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (currentScene) {

	case 0:
		/*********************************************************************
		*								0.
		*Render dummy scene to current window (Standard Shader)
		*********************************************************************/
		glClearColor(1.0f, 0.0f, 0.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		//Render Dummy-Scene to current window
		glUseProgram(standardShader);
		uniform(standardShader, "viewMatrix", viewMatrix);
		uniform(standardShader, "projMatrix", projMatrix);
		uniform(standardShader, "modelMatrix", glm::mat4());
		uniform(standardShader, "col", glm::vec3(0, 1, 0));
		teaPot->draw();
		glUseProgram(0);
		break;


	case 1:
		/*********************************************************************
		*								1.
		*Render dummy scene to current window and create FBO with second scene
		*Render the FBO over the dummy scene
		*********************************************************************/
		glClearColor(1.0f, 1.0f, 1.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		//Render Dummy-Scene to current window
		glUseProgram(standardShader);
		uniform(standardShader, "viewMatrix", viewMatrix);
		uniform(standardShader, "projMatrix", projMatrix);
		uniform(standardShader, "modelMatrix", glm::mat4());
		uniform(standardShader, "col", glm::vec3(0, 0, 1));
		teaPot->draw();
		glUseProgram(0);

		//Render Scene into FBO
		fbo->Bind();
		{
			glClearColor(0.2f, 0.2f, 0.2f, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			renderScene(gbufferShader);
			gl_check_error("post teapot error test");
		}
		fbo->Unbind();

		//Render FBO to screen-sized quad
		glUseProgram(standardMiniColorFBOShader);
		//We only use the GL_TEXTURE0 for the shader, the others are just an example how to bind multiple 
		//textures
		glActiveTexture(GL_TEXTURE0);
		diffuse->Bind();
		uniform(standardMiniColorFBOShader, "tex", 0);
		uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.4f, 0.6f));
		uniform(standardMiniColorFBOShader, "upRight", glm::vec2(0.8f, 0.9f));
		quad->draw();

		glActiveTexture(GL_TEXTURE1);
		normal->Bind();
		uniform(standardMiniColorFBOShader, "tex", 1);
		uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.0f, 0.0f));
		uniform(standardMiniColorFBOShader, "upRight", glm::vec2(0.15f, 0.15f));
		quad->draw();

		glUseProgram(0);
		diffuse->Unbind();
		normal->Unbind();
		position->Unbind();
		depth->Unbind();


		break;


	case 2:
		/*********************************************************************
		*								2.
		*							Sky-Dome
		*********************************************************************/

		glClearColor(1.0f, 1.0f, 1.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glUseProgram(standardPhongShader);

		

		uniform(standardPhongShader, "viewMatrix", viewMatrix);
		uniform(standardPhongShader, "projMatrix", projMatrix);
		uniform(standardPhongShader, "ambientColor", glm::vec3(0.1f));
		uniform(standardPhongShader, "lightColor", glm::vec3(1.0f));
		uniform(standardPhongShader, "specular", 1.0f);
		uniform(standardPhongShader, "glossiness", 50.0f);
		uniform(standardPhongShader, "metalness", 0.0f);
		uniform(standardPhongShader, "cameraPosition", glm::vec3(cam.position));
		uniform(standardPhongShader, "lightPosition", lightDir);
		uniform(standardPhongShader, "lightType", 1);

		thisModelMatrix = glm::mat4();
		uniform(standardPhongShader, "albedoColor", glm::vec3(0, 0, 1));
		uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
		normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
		uniform(standardPhongShader, "normalMatrix", normalMatrix);
		teaPot->draw();

		thisModelMatrix = glm::translate(glm::vec3(5.0f, 0.0f, 0.0f));
		uniform(standardPhongShader, "albedoColor", glm::vec3(0, 1, 0));
		uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
		normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
		uniform(standardPhongShader, "normalMatrix", normalMatrix);
		teaPot->draw();

		thisModelMatrix = glm::translate(glm::vec3(-5.0f, 0.0f, 0.0f));
		uniform(standardPhongShader, "albedoColor", glm::vec3(1, 0, 0));
		uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
		normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
		uniform(standardPhongShader, "normalMatrix", normalMatrix);
		teaPot->draw();

		thisModelMatrix = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
		uniform(standardPhongShader, "albedoColor", glm::vec3(0.5));
		uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
		normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
		uniform(standardPhongShader, "normalMatrix", normalMatrix);
		plane->draw();
		glUseProgram(0);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, envMap);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderMap(mapDomeShader);
		glDisable(GL_CULL_FACE);

		break;

	case 3:
		/*********************************************************************
		*								3.
		*Render normal and depth into tow different FBO's
		*Render all 4 into the screen
		*
		*********************************************************************/
		/*
		*	Render-Front-Face
		*/
		fboNormalDepth->Bind();
		{
			glClearColor(0.2f, 0.2f, 0.2f, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			glUseProgram(fboNormalDepthShader);
			uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
			uniform(fboNormalDepthShader, "projMatrix", projMatrix);
			// render pots
			for (int i = 0; i < 10; ++i) {
				float rad = ((float)i / 10.f) * M_PI * 2;
				float dist = 8.f;
				float x = cos(rad)*dist;
				float y = sin(rad)*dist;
				uniform(fboNormalDepthShader, "modelMatrix", glm::translate(glm::vec3(x, 0, y)) * glm::scale(glm::mat4(), glm::vec3(2.5, 2.5, 2.5)));
				teaPot->draw();
			}

			// render ground plane
			uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
			plane->draw();
			glUseProgram(0);

			gl_check_error("post teapot error test");
		}
		fboNormalDepth->Unbind();

		glUseProgram(standardMiniDepthFBOShader);
		glActiveTexture(GL_TEXTURE0);
		fboND_depth->Bind();
		uniform(standardMiniDepthFBOShader, "tex", 0);
		uniform(standardMiniDepthFBOShader, "downLeft", glm::vec2(0.0f, 0.0f));
		uniform(standardMiniDepthFBOShader, "upRight", glm::vec2(0.5f, 0.5f));
		quad->draw();
		fboND_depth->Unbind();
		glUseProgram(0);

		glUseProgram(standardMiniColorFBOShader);
		glActiveTexture(GL_TEXTURE0);
		fboND_normal->Bind();
		uniform(standardMiniColorFBOShader, "tex", 0);
		uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.0f, 0.5f));
		uniform(standardMiniColorFBOShader, "upRight", glm::vec2(0.5f, 1.0f));
		quad->draw();
		fboND_normal->Unbind();
		glUseProgram(0);

		/*
		*	Render-Back-Face
		*/
		fboNormalDepth2->Bind();
		{
			glClearColor(0.2f, 0.2f, 0.2f, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);

			glUseProgram(fboNormalDepthShader);
			uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
			uniform(fboNormalDepthShader, "projMatrix", projMatrix);
			// render pots
			for (int i = 0; i < 10; ++i) {
				float rad = ((float)i / 10.f) * M_PI * 2;
				float dist = 8.f;
				float x = cos(rad)*dist;
				float y = sin(rad)*dist;
				uniform(fboNormalDepthShader, "modelMatrix", glm::translate(glm::vec3(x, 0, y)) * glm::scale(glm::mat4(), glm::vec3(2.5, 2.5, 2.5)));
				teaPot->draw();
			}

			// render ground plane
			uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
			plane->draw();
			glUseProgram(0);
			gl_check_error("post teapot error test");
			glDisable(GL_CULL_FACE);
		}
		fboNormalDepth2->Unbind();

		glUseProgram(standardMiniDepthFBOShader);
		glActiveTexture(GL_TEXTURE0);
		fboND_depth2->Bind();
		uniform(standardMiniDepthFBOShader, "tex", 0);
		uniform(standardMiniDepthFBOShader, "downLeft", glm::vec2(0.5f, 0.0f));
		uniform(standardMiniDepthFBOShader, "upRight", glm::vec2(1.0f, 0.5f));
		quad->draw();
		fboND_depth2->Unbind();
		glUseProgram(0);

		glUseProgram(standardMiniColorFBOShader);
		glActiveTexture(GL_TEXTURE0);
		fboND_normal2->Bind();
		uniform(standardMiniColorFBOShader, "tex", 0);
		uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.5f, 0.5f));
		uniform(standardMiniColorFBOShader, "upRight", glm::vec2(1.0f, 1.0f));
		quad->draw();
		fboND_normal2->Unbind();
		glUseProgram(0);
		break;

		case 4:
			/*********************************************************************
			*								4.
			*Render normal and depth into tow different FBO's
			*Render all 4 into the screen
			*
			*********************************************************************/
			/*
			*	Render-Front-Face
			*/
			fboNormalDepth->Bind();
			{
				glClearColor(0.2f, 0.2f, 0.2f, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glUseProgram(fboNormalDepthShader);
				uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
				uniform(fboNormalDepthShader, "projMatrix", projMatrix);
				// render pots
				uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
				teaPot->draw();
				glUseProgram(0);

				gl_check_error("post teapot error test");
			}
			fboNormalDepth->Unbind();

			glUseProgram(standardMiniDepthFBOShader);
			glActiveTexture(GL_TEXTURE0);
			fboND_depth->Bind();
			uniform(standardMiniDepthFBOShader, "tex", 0);
			uniform(standardMiniDepthFBOShader, "downLeft", glm::vec2(0.0f, 0.0f));
			uniform(standardMiniDepthFBOShader, "upRight", glm::vec2(0.5f, 0.5f));
			quad->draw();
			fboND_depth->Unbind();
			glUseProgram(0);

			glUseProgram(standardMiniColorFBOShader);
			glActiveTexture(GL_TEXTURE0);
			fboND_normal->Bind();
			uniform(standardMiniColorFBOShader, "tex", 0);
			uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.0f, 0.5f));
			uniform(standardMiniColorFBOShader, "upRight", glm::vec2(0.5f, 1.0f));
			quad->draw();
			fboND_normal->Unbind();
			glUseProgram(0);

			/*
			*	Render-Back-Face
			*/
			fboNormalDepth2->Bind();
			{
				glClearColor(0.2f, 0.2f, 0.2f, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);

				glUseProgram(fboNormalDepthShader);
				uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
				uniform(fboNormalDepthShader, "projMatrix", projMatrix);
				// render pots
				uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
				teaPot->draw();
				glUseProgram(0);
				gl_check_error("post teapot error test");
				glDisable(GL_CULL_FACE);
			}
			fboNormalDepth2->Unbind();

			glUseProgram(standardMiniDepthFBOShader);
			glActiveTexture(GL_TEXTURE0);
			fboND_depth2->Bind();
			uniform(standardMiniDepthFBOShader, "tex", 0);
			uniform(standardMiniDepthFBOShader, "downLeft", glm::vec2(0.5f, 0.0f));
			uniform(standardMiniDepthFBOShader, "upRight", glm::vec2(1.0f, 0.5f));
			quad->draw();
			fboND_depth2->Unbind();
			glUseProgram(0);

			glUseProgram(standardMiniColorFBOShader);
			glActiveTexture(GL_TEXTURE0);
			fboND_normal2->Bind();
			uniform(standardMiniColorFBOShader, "tex", 0);
			uniform(standardMiniColorFBOShader, "downLeft", glm::vec2(0.5f, 0.5f));
			uniform(standardMiniColorFBOShader, "upRight", glm::vec2(1.0f, 1.0f));
			quad->draw();
			fboND_normal2->Unbind();
			glUseProgram(0);

			//glm::vec3 viewDirection = cam.viewDir;
			//glm::vec3 viewPoint = cam.position;

			//float outsideMaterial = 1.0f; //Air
			//float insideMaterial = 1.33f; //Water

			////Critical angle is only relevant when light moves from n_i < n_t
			//float criticalAngle = glm::asin(outsideMaterial / insideMaterial);

			break;

			case 5:
				/*********************************************************************
				*								5.
				*						Refraction
				*
				*********************************************************************/
				
				//FBO-Frontface
				fboNormalDepth->Bind();
				{
					glClearColor(0.2f, 0.2f, 0.2f, 1);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glEnable(GL_DEPTH_TEST);
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);

					glUseProgram(fboNormalDepthShader);
					uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
					uniform(fboNormalDepthShader, "projMatrix", projMatrix);
					// render pots
					uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
					teaPot->draw();
					glUseProgram(0);

					gl_check_error("post teapot error test");
				}
				fboNormalDepth->Unbind();

				//FBO-Backface
				fboNormalDepth2->Bind();
				{
					glClearColor(0.2f, 0.2f, 0.2f, 1);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glEnable(GL_DEPTH_TEST);
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);

					glUseProgram(fboNormalDepthShader);
					uniform(fboNormalDepthShader, "viewMatrix", viewMatrix);
					uniform(fboNormalDepthShader, "projMatrix", projMatrix);
					// render pots
					uniform(fboNormalDepthShader, "modelMatrix", glm::mat4());
					teaPot->draw();
					glUseProgram(0);
					gl_check_error("post teapot error test");
					glDisable(GL_CULL_FACE);
				}
				fboNormalDepth2->Unbind();

				glClearColor(1.0f, 1.0f, 1.0f, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);

				glUseProgram(standardPhongShader);

				uniform(standardPhongShader, "viewMatrix", viewMatrix);
				uniform(standardPhongShader, "projMatrix", projMatrix);
				uniform(standardPhongShader, "ambientColor", glm::vec3(0.1f));
				uniform(standardPhongShader, "lightColor", glm::vec3(1.0f));
				uniform(standardPhongShader, "specular", 1.0f);
				uniform(standardPhongShader, "glossiness", 50.0f);
				uniform(standardPhongShader, "metalness", 0.0f);
				uniform(standardPhongShader, "cameraPosition", glm::vec3(cam.position));
				uniform(standardPhongShader, "lightPosition", lightDir);
				uniform(standardPhongShader, "lightType", 1);

				thisModelMatrix = glm::mat4();
				uniform(standardPhongShader, "albedoColor", glm::vec3(0, 0, 1));
				uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
				normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
				uniform(standardPhongShader, "normalMatrix", normalMatrix);
				teaPot->draw();

				thisModelMatrix = glm::translate(glm::vec3(5.0f, 0.0f, 0.0f));
				uniform(standardPhongShader, "albedoColor", glm::vec3(0, 1, 0));
				uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
				normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
				uniform(standardPhongShader, "normalMatrix", normalMatrix);
				teaPot->draw();

				thisModelMatrix = glm::translate(glm::vec3(-5.0f, 0.0f, 0.0f));
				uniform(standardPhongShader, "albedoColor", glm::vec3(1, 0, 0));
				uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
				normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
				uniform(standardPhongShader, "normalMatrix", normalMatrix);
				teaPot->draw();

				thisModelMatrix = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
				uniform(standardPhongShader, "albedoColor", glm::vec3(0.5));
				uniform(standardPhongShader, "modelMatrix", thisModelMatrix);
				normalMatrix = glm::transpose(glm::inverse(thisModelMatrix));
				uniform(standardPhongShader, "normalMatrix", normalMatrix);
				plane->draw();
				glUseProgram(0);


				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, envMap);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				renderMap(mapDomeShader);
				glDisable(GL_CULL_FACE);

			break;
	}

	
	/*
	// shade with primary light source
	//{
	//	glClearColor(0.2f, 0.2f, 0.2f, 1);
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//	glUseProgram(quadShader);

	//	glActiveTexture(GL_TEXTURE0);
	//	diffuse->Bind();
	//	uniform(quadShader, "diffuse", 0);

	//	glActiveTexture(GL_TEXTURE1);
	//	normal->Bind();
	//	uniform(quadShader, "normal", 1);

	//	glActiveTexture(GL_TEXTURE2);
	//	position->Bind();
	//	uniform(quadShader, "position", 2);

	//	glActiveTexture(GL_TEXTURE3);
	//	depth->Bind();
	//	uniform(quadShader, "depth", 3);

	//	uniform(quadShader, "debug", debugShading);

	//	uniform(quadShader, "lightDir", lightDir);
	//	uniform(quadShader, "viewMatrix", viewMatrix);

	//	quad->draw();

	//	if (!debugShading) {
	//		drawSplats(gbufferShader, 0.1f, false);
	//	}

	//}

	//// generate splat stencil buffer
	//if (stencilCulling && !debugShading)
	//{
	//	glUseProgram(stencilShader);
	//	glClear(GL_STENCIL_BUFFER_BIT);
	//	glEnable(GL_STENCIL_TEST); // first clear stencil buffer by writing default stencil value (0) to all of stencil buffer.

	//							   // TODO A1 (b), setup stencil buffer
	//							   // https://en.wikibooks.org/wiki/OpenGL_Programming/Stencil_buffer

	//							   //                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	//							   //                glDepthMask(GL_FALSE);
	//							   //                glStencilFunc(GL_NEVER, 1, 0xFF); //never pass stencil test
	//							   //                glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);  // draw 1s on test fail (always),
	//							   //                glStencilMask(0xFF);
	//							   //                glClear(GL_STENCIL_BUFFER_BIT);  // needs mask=0xFF

	//							   //glCullFace(GL_FRONT);

	//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //disable color rendering

	//	glDepthMask(GL_FALSE);
	//	glDepthFunc(GL_LESS); //disable depth buffer writes

	//	glStencilFunc(GL_ALWAYS, 1, 0xFF); //always pass stencil test //if depth test fails, replace depth value with ref (=1)
	//	glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);

	//	drawSplats(stencilShader, radius, true);

	//	// TODO A1 (b), reset to opengl default values

	//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	//	glDepthFunc(GL_LESS);
	//	glDepthMask(GL_TRUE);
	//	glStencilFunc(GL_EQUAL, 1, 0xFF);
	//	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


	//	gl_check_error("in stencil");

	//	//stencil debug
	//	if (debugStencil) {
	//		if (clearDebugStencil) {
	//			glClearColor(0.f, 0.f, 0.f, 1);
	//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//		}

	//		// TODO A1 (b), setup depth and stencil setting for masking
	//		glDepthFunc(GL_ALWAYS);
	//		glStencilFunc(GL_EQUAL, 1, 0xFF); //enable color rendering //always write depth//only render if set by prev. pass //don’t touch stencil values
	//		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	//		drawSplats(stencilShader, radius, true);

	//		glUseProgram(stencilDebugShader);
	//		quad->draw();

	//		glDepthFunc(GL_LESS);
	//	}
	//	gl_check_error("after stencil");
	//}


	//// actual splatting
	//// TODO A1 (b) & (c), take care of correct culling modes and appropriate resetting
	//if (!debugStencil && splatting && !debugShading)
	//{
	//	glDepthMask(false);
	//	glEnable(GL_CULL_FACE);
	//	if (stencilCulling) {
	//		glDepthFunc(GL_ALWAYS);
	//		glStencilFunc(GL_EQUAL, 1, ~0);
	//		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	//	}

	//	glUseProgram(splatShader);
	//	uniform(splatShader, "height", HEIGHT);
	//	uniform(splatShader, "width", WIDTH);

	//	uniform(splatShader, "diffuse", 0);
	//	uniform(splatShader, "normal", 1);
	//	uniform(splatShader, "position", 2);
	//	uniform(splatShader, "depth", 3);
	//	glEnable(GL_BLEND);
	//	glBlendFunc(GL_ONE, GL_ONE);

	//	drawSplats(splatShader, radius, true);

	//	glDisable(GL_BLEND);
	//	glDepthMask(true);
	//	glDisable(GL_CULL_FACE);
	//}

	//gl_check_error("after splatting");

	//glDisable(GL_STENCIL_TEST);
	//glDepthFunc(GL_LESS);

	//depth->Unbind();
	//normal->Unbind();
	//diffuse->Unbind();

	//glUseProgram(0);
	*/
	// tweak bar
	TwDraw();
	glutSwapBuffers();
	glutPostRedisplay();

}

void loadShader(bool init) {
	GLuint shader;

	if (createProgram_VF("./shader/quad.vs.glsl", "./shader/quad.fs.glsl", &shader))
		quadShader = shader;
	else if (init) exit(1);
	gl_check_error("quad shader");

	if (createProgram_VF("./shader/stencil.vs.glsl", "./shader/stencil.fs.glsl", &shader))
		stencilShader = shader;
	else if (init) exit(1);
	gl_check_error("stencil shader");

	if (createProgram_VF("./shader/gbuffer.vs.glsl", "./shader/gbuffer.fs.glsl", &shader))
		gbufferShader = shader;
	else if (init) exit(1);
	gl_check_error("gbuffer shader");

	if (createProgram_VF("./shader/splat.vs.glsl", "./shader/splat.fs.glsl", &shader))
		splatShader = shader;
	else if (init) exit(1);
	gl_check_error("splat shader");

	if (createProgram_VF("./shader/debug.vs.glsl", "./shader/debug.fs.glsl", &shader))
		stencilDebugShader = shader;
	else if (init) exit(1);
	gl_check_error("stencil debug shader");

	if (createProgram_VF("./shader/standard.vs.glsl", "./shader/standard.fs.glsl", &shader))
		standardShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");
	
	if (createProgram_VF("./shader/standardMiniColorFBO.vs.glsl", "./shader/standardMiniColorFBO.fs.glsl", &shader))
		standardMiniColorFBOShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");

	if (createProgram_VF("./shader/standardMiniDepthFBO.vs.glsl", "./shader/standardMiniDepthFBO.fs.glsl", &shader))
		standardMiniDepthFBOShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");

	if (createProgram_VF("./shader/fboNormalDepth.vs.glsl", "./shader/fboNormalDepth.fs.glsl", &shader))
		fboNormalDepthShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");

	if (createProgram_VF("./shader/mapDome.vs.glsl", "./shader/mapDome.fs.glsl", &shader))
		mapDomeShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");
	
	if (createProgram_VF("./shader/standardPhong.vs.glsl", "./shader/standardPhong.fs.glsl", &shader))
		standardPhongShader = shader;
	else if (init) exit(1);
	gl_check_error("standard shader");

	std::cout << "shader loaded" << std::endl;
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);

	glutCreateWindow("Assignment 01");

	setupTweakBar();

	GLenum err = glewInit();
	if (GLEW_OK != err)
		std::cerr << "Error : " << glewGetErrorString(err) << std::endl;

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard); //key pressed down
	//glutKeyboardUpFunc(keyboard); //key released

	glutMotionFunc(onMouseMove);
	glutMouseFunc(onMouseDown);
	glutReshapeFunc(reshape);
	glutIdleFunc(onIdle);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT); //Type-Cast for VisualStudio
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT); //Type-Cast for VisualStudio
	TwGLUTModifiersFunc(glutGetModifiers);

	initGL();

	init();

	glutMainLoop();
	return 0;
}
		






 

