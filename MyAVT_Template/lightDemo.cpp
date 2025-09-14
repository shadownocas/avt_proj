//
// AVT 2025: Texturing with Phong Shading and Text rendered with TrueType library
// The text rendering was based on https://dev.to/shreyaspranav/how-to-render-truetype-fonts-in-opengl-using-stbtruetypeh-1p5k
// You can also learn an alternative with FreeType text: https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: Jo�o Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

#include "renderer.h"
#include "shader.h"
#include "mathUtility.h"
#include "model.h"
#include "texture.h"

using namespace std;

#define CAPTION "AVT 2025 Welcome Demo"
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

//File with the font
const string fontPathFile = "fonts/arial.ttf";

//Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

//Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;
	
// Camera Position
//float camX, camY, camZ;

bool keyStates[256] = {false};

struct Drone {
    float position[3] = {20.0f, 20.0f, -20.0f}; // xyz
    float direction[3] = {0.0f, 0.0f, -1.0f}; // pointing along -Z initially
};

struct FlyingObject {
    float position[3];
    float direction[3];
    float speed;
    float rotationAngle;
    float rotationSpeed;
    int meshID; // which geometry primitive to use
    bool active;
};

std::vector<FlyingObject> flyingObjects;


Drone drone;
float droneSpeed = 0.2f;       // units per frame
float followDistance = 15.0f;  // camera distance behind drone
float followHeight = 5.0f;     // camera height above drone
float droneRotSpeed = 1.5f;    // degrees per frame

struct Camera {
  float camPos[3] = {0.0f, 0.0f, 0.0f};
  float camTarget[3] = {0.0f, 0.0f, 0.0f};
  int type=0; //0:perspective, 1:orthographic
};

Camera cams[3];

int activeCam = 0;

bool lampsOn = true; // all lamps initially on

std::vector<Lamp> lampPositions;


// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 57.0f, beta = 18.0f;
float r = 45.0f;

// Frame counting and FPS computation
long myTime,timebase = 0,frame = 0;
char s[32];

bool dayMode = true;                 // Day/night toggle
float dirLightDir[3] = { -0.5f, -1.0f, -0.3f };  // Direction of sunlight
float dirLightColor[3] = { 1.0f, 1.0f, 0.9f };   // Day color
float nightLightColor[3] = { 0.1f, 0.1f, 0.2f }; // Night ambient color

float lightPos[4] = {4.0f, 20.0f, 2.0f, 1.0f};
//float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

//Spotlight
bool spotlight_mode = false;
float coneDir[4] = { 0.0f, -0.0f, -1.0f, 0.0f };

int rows = 7;           // number of rows
int cols = 7;           // number of columns
float gap = 20.0f;
float offsetX = -((cols-1) * (10.0f + gap)) / 2.0f; // center grid 10 = buildingWidth 
float offsetZ = -((rows-1) * (10.0f + gap)) / 2.0f; // 10 = buildingDepth

int numLamps = std::min(rows, cols); 
float lampHeight = 10.0f;
float lampOffset = (10.0f + gap) / 2.0f + 2.0f; // push them to the side of diagonal 10 = buildingWidth 


bool fontLoaded = false;

struct Building {
    float x, z;
    float width, depth;
};

std::vector<Building> buildings;
std::vector<std::vector<float>> buildingHeights(rows, std::vector<float>(cols));

bool checkOverlap(const Building& a, const Building& b, float buffer = 1.0f) {
    return !(a.x + a.width/2 + buffer < b.x - b.width/2 ||
             a.x - a.width/2 - buffer > b.x + b.width/2 ||
             a.z + a.depth/2 + buffer < b.z - b.depth/2 ||
             a.z - a.depth/2 - buffer > b.z + b.depth/2);
}


/// ::::::::::::::::::::::::::::::::::::::::::::::::CALLBACK FUNCIONS:::::::::::::::::::::::::::::::::::::::::::::::::://///

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void refresh(int value)
{
	glutPostRedisplay();
	glutTimerFunc(1000 / 60, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {
	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	mu.loadIdentity(gmu::PROJECTION);
	if(activeCam == 0){
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
		printf("entra change size prespectiev");
	} else {
		mu.ortho(0, w - 1, 0, h - 1, -1, 1);
	}
}


// ------------------------------------------------------------
//
// Render stufff
//

void dronePosition(){
	if (keyStates['w']){
		drone.position[0] += drone.direction[0] * droneSpeed;
		drone.position[2] += drone.direction[2] * droneSpeed;
	}
	if (keyStates['s']){
		drone.position[0] -= drone.direction[0] * droneSpeed;
		drone.position[2] -= drone.direction[2] * droneSpeed;
	}

	// Rotate left/right (yaw around y-axis)
	if (keyStates['a']){
		float angle = -droneRotSpeed * 3.14159f / 180.0f;
		float cosA = cos(angle);
		float sinA = sin(angle);
		float dx = drone.direction[0];
		float dz = drone.direction[2];
		drone.direction[0] = dx * cosA - dz * sinA;
		drone.direction[2] = dx * sinA + dz * cosA;
	}
	if (keyStates['d']){
		float angle = droneRotSpeed * 3.14159f / 180.0f;
		float cosA = cos(angle);
		float sinA = sin(angle);
		float dx = drone.direction[0];
		float dz = drone.direction[2];
		drone.direction[0] = dx * cosA - dz * sinA;
		drone.direction[2] = dx * sinA + dz * cosA;
	}
}

void updateCamera(){
	float ratio = (float)WinX / (float)WinY;

	mu.loadIdentity(gmu::PROJECTION);

	if (activeCam == 0 || activeCam == 2) {
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	} else if (activeCam == 1) {
		float orthoSize = 60.0f;
		mu.ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, -500.0f, 500.0f);
	}
}
void renderSim(void) {

    FrameCount++;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination

    renderer.setTexUnit(0, 0);
    renderer.setTexUnit(1, 1);
    renderer.setTexUnit(2, 2);
    renderer.setTexUnit(3, 3);
    renderer.setTexUnit(4, 4);

    // Update drone movement
    dronePosition();
    updateCamera();

    // --- Set camera/view ---
    mu.loadIdentity(gmu::VIEW);
    mu.loadIdentity(gmu::MODEL);
    mu.lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
              cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2], 0,1,0);

    // --- Light positions ---
    float lposAux[4];
    mu.multMatrixPoint(gmu::VIEW, lightPos, lposAux);   
    renderer.setLightPos(lposAux);

	// ----- UPDATE FLYING OBJECTS -----
	for (auto &obj : flyingObjects) {
		// Move forward
		obj.position[0] += obj.direction[0] * obj.speed;
		obj.position[1] += obj.direction[1] * obj.speed;
		obj.position[2] += obj.direction[2] * obj.speed;

		// Rotate while moving
		obj.rotationAngle += obj.rotationSpeed;
		if (obj.rotationAngle > 360.0f) obj.rotationAngle -= 360.0f;

		// Increase speed slightly with play time
		obj.speed *= 1.0001f;

		// Respawn if out of visible region
		if (fabs(obj.position[0]) > 150 || fabs(obj.position[2]) > 150) {
			obj.position[0] = (rand() % 200 - 100);
			obj.position[1] = 10.0f + rand() % 30;
			obj.position[2] = (rand() % 200 - 100);

			float angle = (rand() % 360) * 3.14159f / 180.0f;
			obj.direction[0] = cos(angle);
			obj.direction[1] = 0.0f;
			obj.direction[2] = sin(angle);

			obj.speed = 0.05f + (rand() % 10) * 0.01f;
			obj.rotationAngle = 0.0f;
		}
	}

	// load identity matrices
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);
	// set the camera using a function similar to gluLookAt
	mu.lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
	cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2], 0,1,0);
    // Spotlight settings
    renderer.setSpotLightMode(spotlight_mode);
    renderer.setSpotParam(coneDir, 2.0f);

    // Directional light
    float dirLightWorld[4] = { 0.5f, -0.7f, 0.3f, 0.0f };
    float dirLightEye[4];
    mu.multMatrixPoint(gmu::VIEW, dirLightWorld, dirLightEye);
    float dirLightEye3[3] = { dirLightEye[0], dirLightEye[1], dirLightEye[2] };

    float lightColor[3] = {1.0f, 1.0f, 0.9f};
    if(!dayMode) {
        lightColor[0] = 0.1f; 
        lightColor[1] = 0.1f; 
        lightColor[2] = 0.2f;
    }
    renderer.setDirectionalLight(dirLightEye3, lightColor);

    // --- Build lamp positions before sending to shader ---
    lampPositions.clear();
    for (int i = 0; i < numLamps; ++i) {
        int r = i;
        int c = i;
        float x = offsetX + c * (10.0f + gap) + lampOffset;
        float z = offsetZ + r * (10.0f + gap);
        float y = lampHeight;
        lampPositions.push_back({x, y, z});
    }
	// Send lamp data to shader
    renderer.setLampLights(lampPositions, mu, lampsOn);

	dataMesh data;

		// ----- RENDER FLYING OBJECTS -----
	for (auto &obj : flyingObjects) {
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, obj.position[0], obj.position[1], obj.position[2]);
		mu.rotate(gmu::MODEL, obj.rotationAngle, 0.0f, 1.0f, 0.0f);

		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		data.meshID = obj.meshID;
		data.texMode = 1;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);

		mu.popMatrix(gmu::MODEL);
	}

    // --- Draw floor ---
    mu.pushMatrix(gmu::MODEL);
    mu.scale(gmu::MODEL, 250.0f, 0.1f, 200.0f);
    mu.rotate(gmu::MODEL,-90.0f, 1.0f, 0.0f, 0.0f);
    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    mu.computeNormalMatrix3x3();

    data.meshID = 0;
    data.texMode = 2;
    data.vm = mu.get(gmu::VIEW_MODEL);
    data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
    data.normal = mu.getNormalMatrix();
    renderer.renderMesh(data);
    mu.popMatrix(gmu::MODEL);

    // --- Draw drone ---
    mu.pushMatrix(gmu::MODEL);
    mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);
    float angleY = atan2(drone.direction[0], -drone.direction[2]) * 180.0f / 3.14159f;
    mu.rotate(gmu::MODEL, angleY, 0.0f, 1.0f, 0.0f);
    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    mu.computeNormalMatrix3x3();

    data.meshID = 1;
    data.texMode = 4;
    data.vm = mu.get(gmu::VIEW_MODEL);
    data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
    data.normal = mu.getNormalMatrix();
    renderer.renderMesh(data);
    mu.popMatrix(gmu::MODEL);

    // --- Draw buildings ---
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if ((r + c) % 2 == 0) {
                float x = offsetX + c * (10.0f + gap);
                float z = offsetZ + r * (10.0f + gap);

                mu.pushMatrix(gmu::MODEL);
                mu.translate(gmu::MODEL, x, 0.0f, z);
                mu.scale(gmu::MODEL, 10.0f, buildingHeights[r][c], 10.0f);
                mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
                mu.computeNormalMatrix3x3();

                data.meshID = 1;
                data.texMode = 3;
                data.vm = mu.get(gmu::VIEW_MODEL);
                data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
                data.normal = mu.getNormalMatrix();
                renderer.renderMesh(data);
                mu.popMatrix(gmu::MODEL);
            }
        }
    }

    // --- Draw lamp posts ---
    for (int i = 0; i < numLamps; ++i) {
        float x = lampPositions[i].x;
		float y = lampPositions[i].y;
		float z = lampPositions[i].z;

        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, x, 0.0f, z);
        mu.scale(gmu::MODEL, 0.3f, lampHeight, 0.3f);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        data.meshID = 1;
        data.texMode = 4;
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);

        mu.popMatrix(gmu::MODEL);
    }

    // --- Update third camera to follow drone ---
    cams[2].camPos[0] = drone.position[0] - drone.direction[0] * followDistance;
    cams[2].camPos[1] = drone.position[1] + followHeight;
    cams[2].camPos[2] = drone.position[2] - drone.direction[2] * followDistance;

    cams[2].camTarget[0] = drone.position[0];
    cams[2].camTarget[1] = drone.position[1];
    cams[2].camTarget[2] = drone.position[2];

    glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//


void keyUp(unsigned char key, int x, int y) {
    keyStates[key] = false;
}

void processKeys(unsigned char key, int xx, int yy)
{
	keyStates[key] = true;

	switch(key) {

		case 27:
			glutLeaveMainLoop();
			break;

		case 'l':   //toggle spotlight mode
			if (!spotlight_mode) {
				spotlight_mode = true;
				printf("Point light disabled. Spot light enabled\n");
			}
			else {
				spotlight_mode = false;
				printf("Spot light disabled. Point light enabled\n");
			}
			break;

		/* case 'r':    //reset
			alpha = 57.0f; beta = 18.0f;  // Camera Spherical Coordinates
			r = 45.0f;
			camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
			camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
			camY = r * sin(beta * 3.14f / 180.0f);
			break; */

		case 'c':   // toggle lamp posts
			lampsOn = !lampsOn;
			printf("Lamp posts %s\n", lampsOn ? "ON" : "OFF");
			break;

		case 'n':   // toggle day/night mode
			dayMode = !dayMode;
			if (dayMode)
				printf("Day mode ON\n");
			else
				printf("Night mode ON\n");
			break;


		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'p': glDisable(GL_MULTISAMPLE); break;
		case '1':
			activeCam = 0; 
			break;
		case '2':
			activeCam = 1;
			break;
		case '3':
			activeCam = 2;
			break;
	}
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX =  - xx + startX;
	deltaY =    yy - startY;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}

	/* camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 10.14f / 180.0f);
	camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 10.14f / 180.0f);
	camY = rAux *   						       sin(betaAux * 10.14f / 180.0f);
 */
//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	/* camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f); */

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


//
// Scene building with basic geometry
//

void buildScene()
{
	//Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/stone.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");
	renderer.TexObjArray.texture2D_Loader("assets/Bricks097.tga");  
	renderer.TexObjArray.texture2D_Loader("assets/metal.tga");  

	//Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
	int texcount = 0;

	// create geometry and VAO of the quad
	amesh = createQuad(1, 1);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	amesh = createCube();
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	
	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			buildingHeights[r][c] = 5.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
		}
	}



	printf("\nNumber of Texture Objects is %d\n\n", renderer.TexObjArray.getNumTextureObjects());

	drone.position[0] = 20.0f;
	drone.position[1] = 20.0f;
	drone.position[2] = -20.0f;

	// For example, pointing along negative z axis initially
	drone.direction[0] = 0.0f;
	drone.direction[1] = 0.0f;
	drone.direction[2] = -1.0f;

	// set the camera position based on its spherical coordinates
	/* camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r * sin(beta * 3.14f / 180.0f); */
	cams[0].camPos[1] = 200.0;
	cams[0].camPos[0] = 0.0;
	cams[0].camPos[2] = 0.33;
	// top ortho
	cams[1].camPos[1] = 200.0;
	cams[1].camPos[2] = 0.33;
	cams[1].type = 1;

	// ----- FLYING OBJECTS GEOMETRY -----
	MyMesh sphere = createSphere(1.0f, 16);
	memcpy(sphere.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(sphere.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(sphere.mat.specular, spec1, 4 * sizeof(float));
	memcpy(sphere.mat.emissive, emissive, 4 * sizeof(float));
	sphere.mat.shininess = shininess;
	sphere.mat.texCount = texcount;
	renderer.myMeshes.push_back(sphere);

	MyMesh cone = createCone(2.0f, 1.0f, 20);
	memcpy(cone.mat.ambient, amb, 4 * sizeof(float));
	memcpy(cone.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(cone.mat.specular, spec, 4 * sizeof(float));
	memcpy(cone.mat.emissive, emissive, 4 * sizeof(float));
	cone.mat.shininess = shininess;
	cone.mat.texCount = texcount;
	renderer.myMeshes.push_back(cone);

	// ----- INITIALIZE FLYING OBJECTS -----
	for (int i = 0; i < 10; i++) {
		FlyingObject obj;

		// random spawn position in XZ plane
		obj.position[0] = (rand() % 200 - 100);
		obj.position[1] = 10.0f + rand() % 30;   // altitude between 10–40
		obj.position[2] = (rand() % 200 - 100);

		// random direction in XZ plane
		float angle = (rand() % 360) * 3.14159f / 180.0f;
		obj.direction[0] = cos(angle);
		obj.direction[1] = 0.0f;
		obj.direction[2] = sin(angle);

		obj.speed = 0.05f + (rand() % 10) * 0.01f;   // 0.05 – 0.15
		obj.rotationAngle = 0.0f;
		obj.rotationSpeed = 1.0f + (rand() % 5);     // 1 – 5 degrees per frame
		obj.meshID = 2 + (i % 2);                    // 2 = sphere, 3 = cone
		obj.active = true;

		flyingObjects.push_back(obj);
	}
}

// ------------------------------------------------------------
//
// Main function
//

int main(int argc, char **argv) {

//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);

//  Callback Registration
	glutDisplayFunc(renderSim);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	glutIdleFunc(renderSim);  // Use it for maximum performance
	//glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutKeyboardUpFunc(keyUp);

	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	srand((unsigned int)time(nullptr)); //randomize seed

	buildScene();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}



