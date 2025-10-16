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
#include <array>

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
#include <cmath>	 // sinf, cosf, atan2f

using namespace std;

#define CAPTION "AVT 2025 Welcome Demo"
#define LAMP_POST_NUMBER 6
#define frand()			((float)rand()/RAND_MAX)
#define M_PI			3.14159265
#define MAX_PARTICULAS  1500

const string fontPathFile = "fonts/arial.ttf";
bool fontLoaded = false;

int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

// Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

// Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;

// Follow-camera mouse offsets
float followYawOffsetDeg = 0.f;
float followPitchOffsetDeg = 15.f; // slight downward tilt by default

// Zoom sensitivity (world units per pixel of RMB drag)
float zoomCam1 = 200.0f;  // start height for top view
float zoomCam2 = 60.0f;   // ortho size

int prevX = 0, prevY = 0;

// Key states
bool spKeys[256] = {false};
bool keyStates[256] = {false};

enum DroneMode {
    NORMAL,
    COLLISION,
};

struct Drone
{
	float position[3] = {20.0f, 20.0f, -20.0f};
	float direction[3] = {0.0f, 0.0f, -1.0f};
	float rotation[3] = {0.0f, 0.0f, 0.0f};
	float speed;
	float yaw;
	float velRot;
	AABB aabb;
	AABB worldAABB;
	DroneMode mode = NORMAL;

	float battery;
    int points;
};

struct Package
{
	float position[3] = {20.0f, 20.0f, -20.0f};
	float direction[3] = {0.0f, 0.0f, -1.0f};
	float rotation[3] = {0.0f, 0.0f, 0.0f};
	float speed;
	float yaw;
	float velRot;
	AABB aabb;
	AABB worldAABB;
	DroneMode mode = NORMAL;
};

struct FlyingObject
{
	float position[3];
	float direction[3];
	float speed;
	float rotationAngle;
	float rotationSpeed;
	int meshID; // which geometry primitive to use
	bool active;
	AABB aabb;
	AABB worldAABB;
};

struct LampPost
{
	float position[3];
	float height;
	AABB aabb;
	AABB worldAABB;
};

struct Tree {
    float position[3];
    float scale[3];
    AABB aabb;     
    AABB worldAABB;
};

struct Building
{
	float position[3];
	float width, depth;
	int row, col;
	bool goal = false;
	AABB aabb;
	AABB worldAABB;
};

struct FloorObject
{
	AABB aabb;
	AABB worldAABB;
} floorObj;

struct Camera
{
	float camPos[3] = {0.0f, 0.0f, 0.0f};
	float camTarget[3] = {0.0f, 0.0f, 0.0f};
	int type = 0; // 0 and 2:perspective, 1:orthographic
};

struct Billboard {
    float position[3];
    float scale[3];
    int textureID;
	AABB aabb;
	AABB worldAABB;
};

int fireworks = 0;

struct Particle {
	float	life;		// vida
	float	fade;		// fade
	GLfloat x, y, z;    // posi��o
	GLfloat vx, vy, vz; // velocidade 
	GLfloat ax, ay, az; // acelera��o
};

Particle particles[1500];
int dead_num_particles = 0;

MeshCollection allMeshes;

Package package;

std::vector<PointLight> pointLights;

std::vector<FlyingObject> flyingObjects;

std::vector<LampPost> lampPosts;

std::vector<Tree> trees;

std::vector<Lamp> lampPositions;

std::vector<Building> buildings;

std::vector<Billboard> billboards;

Drone drone;
float followDistance = 15.0f;
float startFollowDistance = 20.f;
float followHeight = 5.0f;

float animationCollision = 0.0f;
float animationCollisionDuration = 0.5f;

Camera cams[4];
int activeCam = 0;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 57.0f;

bool lampsOn = true;

char s[32];

// Ambient light
bool dayMode = true;
float dirLightColor[3] = {1.0f, 1.0f, 0.9f};
float nightLightColor[3] = {0.0f, 0.0f, 0.0f};

// Spotlight
bool spotlight_mode = false;
SpotLight droneHeadlights[2]; // two headlights

// City
int rows = 7; // number of rows
int cols = 7; // number of columns
float gap = 20.0f;
float offsetX = -((cols - 1) * (10.0f + gap)) / 2.0f;
float offsetZ = -((rows - 1) * (10.0f + gap)) / 2.0f;
std::vector<std::vector<float>> buildingHeights(rows, std::vector<float>(cols));

float lampHeight = 10.0f;

float buildingW = 10.0f;
float buildingD = 10.0f;
float streetWidth = 30.0f;
float sidewalk = 2.0f;

int gardenSizeRows = 3;
int gardenSizeCols = 3;

float gardenHalfW = (gardenSizeCols * (buildingW + gap) - gap) / 2.0f;
float gardenHalfD = (gardenSizeRows * (buildingD + gap) - gap) / 2.0f;

float gardenCenterX = 6.0f;
float gardenCenterZ = 5.0f;
float gardenW = gardenHalfW * 2.0f;
float gardenD = gardenHalfD * 2.0f;

// Collision
bool flyingColision = false;
bool collisionPackage = false;

// Fog
bool gFogOn = false;
float gFogStart = 60.0f;
float gFogEnd = 180.0f;
float gFogColor[3] = {0.65f, 0.72f, 0.80f};

//Text management
bool pause = false;
bool restart = false;
bool gameOver = false;


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

void changeSize(int w, int h)
{
	float ratio;
	if (h == 0)
		h = 1;

	WinX = w;
	WinY = h;

	glViewport(0, 0, w, h); //viewport is whole window
	ratio = (1.0f * w) / h;
	int m_viewport[4];
	
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	mu.loadIdentity(gmu::PROJECTION);
	if (activeCam == 0 || activeCam == 2)
	{
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	}
	else
	{
		mu.ortho(0, w - 1, 0, h - 1, -1, 1);
	}
}

AABB updateGlobalAABB(AABB result, float* modelMatrix) {
    float globalMin[3] = { 99999999999, 99999999999, 99999999999 };
    float globalMax[3] = { -99999999999, -99999999999, -99999999999 };
	AABB obj = result; 
    for (int i = 0; i < 8; i++) {
        float* c = obj.corners[i];

        float x = modelMatrix[0] * c[0] + modelMatrix[4] * c[1] + modelMatrix[8]  * c[2] + modelMatrix[12];
        float y = modelMatrix[1] * c[0] + modelMatrix[5] * c[1] + modelMatrix[9]  * c[2] + modelMatrix[13];
        float z = modelMatrix[2] * c[0] + modelMatrix[6] * c[1] + modelMatrix[10] * c[2] + modelMatrix[14];
        float w = modelMatrix[3] * c[0] + modelMatrix[7] * c[1] + modelMatrix[11] * c[2] + modelMatrix[15];

        if (w != 0.0f) {
            x /= w;
            y /= w;
            z /= w;
        }

        obj.corners[i][0] = x;
        obj.corners[i][1] = y;
        obj.corners[i][2] = z;

		// Update global max
        globalMax[0] = std::max(globalMax[0], x);
        globalMax[1] = std::max(globalMax[1], y);
        globalMax[2] = std::max(globalMax[2], z);

        // Update global min
        globalMin[0] = std::min(globalMin[0], x);
        globalMin[1] = std::min(globalMin[1], y);
        globalMin[2] = std::min(globalMin[2], z);
    }

    obj.aabbmin[0] = globalMin[0];
    obj.aabbmin[1] = globalMin[1];
    obj.aabbmin[2] = globalMin[2];

    obj.aabbmax[0] = globalMax[0];
    obj.aabbmax[1] = globalMax[1];
    obj.aabbmax[2] = globalMax[2];

    return obj;
}

AABB mergeAABBs(const AABB& a, const AABB& b) {
    AABB merged;
    merged.aabbmin[0] = std::min(a.aabbmin[0], b.aabbmin[0]);
    merged.aabbmin[1] = std::min(a.aabbmin[1], b.aabbmin[1]);
    merged.aabbmin[2] = std::min(a.aabbmin[2], b.aabbmin[2]);

    merged.aabbmax[0] = std::max(a.aabbmax[0], b.aabbmax[0]);
    merged.aabbmax[1] = std::max(a.aabbmax[1], b.aabbmax[1]);
    merged.aabbmax[2] = std::max(a.aabbmax[2], b.aabbmax[2]);

    return merged;
}

// ------------------------------------------------------------
//
// Render stufff
//


void iniParticles(void)
{
    float v, theta, phi;
    int i;

    for (i = 0; i < MAX_PARTICULAS; i++) {
        v = 0.8f * frand() + 0.2f;
        phi = frand() * M_PI;
        theta = frand() * 2.0f * M_PI;

        particles[i].x = drone.position[0];
        particles[i].y = drone.position[1];
        particles[i].z = drone.position[2];

        particles[i].vx = v * cosf(theta) * sinf(phi);
        particles[i].vy = v * cosf(phi);
        particles[i].vz = v * sinf(theta) * sinf(phi);

        particles[i].ax = 0.1f;
        particles[i].ay = -0.15f;
        particles[i].az = 0.0f;

        particles[i].life = 1.0f;
        particles[i].fade = 0.0025f;
    }
}

void randomPackagePos(){
    int randomIndex = rand() % buildings.size();
    Building& chosen = buildings[randomIndex];

    int goalIndex;
    do {
        goalIndex = rand() % buildings.size();
    } while (goalIndex == randomIndex);

    buildings[goalIndex].goal = true;

    float height = buildingHeights[chosen.row][chosen.col];
    package.position[0] = chosen.position[0] + chosen.width / 2.0f - 0.5f; // subtract half package size
    package.position[1] = height;
    package.position[2] = chosen.position[2] + chosen.depth / 2.0f - 1.0f;
}


void restartPackage(){
	randomPackagePos();

	package.rotation[0] = 0.0f;
	package.rotation[1] = 0.0f;
	package.rotation[2] = 0.0f;

	package.speed = 0.0f;
}

void restartDrone(){
	drone.position[0] = 20.0f;
	drone.position[1] = 20.0f;
	drone.position[2] = -20.0f;

	drone.rotation[0] = 0.0f;
	drone.rotation[1] = 0.0f;
	drone.rotation[2] = 0.0f;

	drone.speed = 0.0f;
}

void restartGoal(){
	for (Building& b : buildings) {
        if (b.goal) {
            b.goal = false;
            break; // assuming only one goal at a time
        }
    }
}

void rotateDrone(float x, float y, float z, float dt) {
	drone.rotation[0] += x * dt;
	drone.rotation[1] += drone.velRot * dt;
	drone.rotation[2] += z * dt;
}

void moveDrone(float dt) {
	float prevPos[3] = { drone.position[0], drone.position[1], drone.position[2] };
	drone.position[0] += drone.direction[0] * drone.speed * dt;
	drone.position[1] += drone.direction[1] * drone.speed * dt;
	drone.position[2] += drone.direction[2] * drone.speed * dt;

	if(drone.battery <= 0.0f){
		drone.speed = 0.0f;
		drone.position[0] = prevPos[0];
		drone.position[1] -= 0.1f;
		drone.position[2] = prevPos[2];
	}
}

void turnBillBoard(float *cam, float *worldPos) {
    float lookAt[3]={0,0,1},objToCamProj[3],objToCam[3],upAux[3],angleCosine;

    objToCamProj[0] = cam[0] - worldPos[0];
    objToCamProj[1] = 0;
    objToCamProj[2] = cam[2] - worldPos[2];
    mu.normalize(objToCamProj);
    mu.crossProduct(lookAt, objToCamProj, upAux);

    angleCosine = mu.dotProduct(lookAt, objToCamProj);

    if ((angleCosine < 0.99990) && (angleCosine > -0.9999)) {
        mu.rotate(gmu::MODEL, acos(angleCosine)*180/3.14,upAux[0], upAux[1], upAux[2]);
    }

    objToCam[0] = cam[0] - worldPos[0];
    objToCam[1] = cam[1] - worldPos[1];
    objToCam[2] = cam[2] - worldPos[2];

    mu.normalize(objToCam);

    angleCosine = mu.dotProduct(objToCamProj, objToCam);

    if ((angleCosine < 0.99990) && (angleCosine > -0.9999)) {
        if (objToCam[1] < 0) {
            mu.rotate(gmu::MODEL,acos(angleCosine)*180/3.14,1,0,0);
        }
        else {
            mu.rotate(gmu::MODEL,acos(angleCosine)*180/3.14,-1,0,0);
        }
    }
}

void updateParticles()
{
	int i;
	float h;

	h = 0.033;
	if (fireworks) {

		for (i = 0; i < MAX_PARTICULAS; i++)
		{
			particles[i].x += (h*particles[i].vx);
			particles[i].y += (h*particles[i].vy);
			particles[i].z += (h*particles[i].vz);
			particles[i].vx += (h*particles[i].ax);
			particles[i].vy += (h*particles[i].ay);
			particles[i].vz += (h*particles[i].az);
			particles[i].life -= particles[i].fade;
		}
	}
}

void updateDroneMovement(float dirXAdd, float dirZAdd, float rotXAdd, float rotZAdd) {
    if (drone.speed < 12.0f) drone.speed += 1.0f;

    // Vertical movement
    drone.direction[1] -= 0.005f;
    float yawRad = mu.DegToRad(drone.rotation[1]);

    drone.direction[0] += dirXAdd;
    drone.direction[2] += dirZAdd;

    // Rotation tilt
    float rotX = drone.rotation[0] + rotXAdd;
    float rotZ = drone.rotation[2] + rotZAdd;
    float tiltMag = sqrtf(rotX * rotX + rotZ * rotZ);
    if (tiltMag > 20.0f) {
        float scale = 20.0f / tiltMag;
        rotX *= scale;
        rotZ *= scale;
    }
    drone.rotation[0] = rotX;
    drone.rotation[2] = rotZ;
}

void manageBattery(float dt){
	float drainRate = 1.0f;
	drone.battery -= (drone.speed / 4.0f) * drainRate * dt; //4 is max speed FIX

	if (drone.battery < 0.0f) drone.battery = 0.0f;

	if (restart) {
		drone.battery = 100.0f;
		restart = false;
		collisionPackage = false;
		restartDrone();
		restartGoal();
		restartPackage();
	}
}

void updateDrone(float dt) {
	const float MAX_VSPEED = 0.1f;

	drone.rotation[1] = fmodf(drone.rotation[1], 360.0f);
	if (drone.rotation[1] < 0.0f) drone.rotation[1] += 360.0f;
	if (drone.speed == 0) {
		drone.direction[0] = 0.0f;
		drone.direction[1] = 0.0f;
		drone.direction[2] = 0.0f;
	}

	if (keyStates['w']) {
		if (drone.speed < 4.0f) drone.speed += MAX_VSPEED;
		if (drone.direction[1] < 1.0f) drone.direction[1] += MAX_VSPEED;
	}

	if (keyStates['s']) {
		if (drone.speed < 4.0f) drone.speed += MAX_VSPEED;
		if (drone.direction[1] > -1.0f) drone.direction[1] -= MAX_VSPEED;

	}

	if (keyStates['a']) {
		if (drone.velRot < 20.0) drone.velRot += 2.0f;
		rotateDrone(0.0f, 3.0f, 0.0f, dt);
	}
	if (keyStates['d']) {
		if (drone.velRot > -20.0) drone.velRot -= 2.0f;
		rotateDrone(0.0f, -3.0f, 0.0f, dt);
	}

	if (spKeys[GLUT_KEY_DOWN]) {
		float yawRad = mu.DegToRad(drone.rotation[1]);
		updateDroneMovement(-sinf(yawRad) * 0.1f, -cosf(yawRad) * 0.1f, -0.5f * cosf(yawRad), 0.5f * sinf(yawRad));
	}

	if (spKeys[GLUT_KEY_UP]) {
		float yawRad = mu.DegToRad(drone.rotation[1]);
		updateDroneMovement(sinf(yawRad) * 0.1f, cosf(yawRad) * 0.1f, 0.5f * cosf(yawRad), -0.5f * sinf(yawRad));
	}

	if (spKeys[GLUT_KEY_LEFT]) {
		float yawRad = mu.DegToRad(drone.rotation[1]);
		updateDroneMovement(cosf(yawRad) * 0.1f, -sinf(yawRad) * 0.1f, -0.5f * sinf(yawRad), -0.5f * cosf(yawRad));
	}

	if (spKeys[GLUT_KEY_RIGHT]) {
		float yawRad = mu.DegToRad(drone.rotation[1]);
		updateDroneMovement(-cosf(yawRad) * 0.1f, sinf(yawRad) * 0.1f, 0.5f * sinf(yawRad), 0.5f * cosf(yawRad));
	}


	float mag = sqrtf(drone.direction[0]*drone.direction[0] + drone.direction[2]*drone.direction[2]);
	if (mag > 1.0f) {
		drone.direction[0] /= mag;
		drone.direction[2] /= mag;
	}

	auto recoverComponent = [](float& x, float& y, float& z, float recoverySpeed) -> float {
		float mag = sqrtf(x*x + z*z + y*y);
		if (mag > 0.0f) {
			float scale = (mag - recoverySpeed) / mag;
			scale = std::max(scale, 0.0f);
			x *= scale;
			z *= scale;
			y *= scale;
			mag *= scale;
		}
		 return mag;
	};

	if (!( spKeys[GLUT_KEY_LEFT] || spKeys[GLUT_KEY_RIGHT] || spKeys[GLUT_KEY_UP] || spKeys[GLUT_KEY_DOWN])) {
		// Recover rotation
		float tempRot = 0.0f;
		float dirMag = 0.0f;
		recoverComponent(drone.rotation[0], drone.rotation[2], tempRot, 0.5f);

		// Recover direction
		if (drone.direction[0] != 0.0f || drone.direction[1] != 0.0f) {
			dirMag = recoverComponent(drone.direction[0], drone.direction[1], drone.direction[2], 0.01f);
		}

		if (dirMag > 0.001f) {
			const float friction = 1.0f;
			drone.speed -= friction * dt;
			if (drone.speed < 0.0f) drone.speed = 0.0f;
		} else {
			drone.speed = 0.0f;
		}
	}

	if (keyStates['a'] == false && keyStates['d'] == false) {
		drone.velRot = 0.0f;
	}

	moveDrone(dt);
}

void updateCameras(){
	float ratio = (float)WinX / (float)WinY;

	mu.loadIdentity(gmu::PROJECTION);

	if (activeCam == 2) {
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	} else if (activeCam == 1 ) {
		mu.ortho(-zoomCam2 * ratio, zoomCam2 * ratio,-zoomCam2, zoomCam2, -500.0f, 500.0f);
	} else if (activeCam == 0) {
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
		cams[0].camPos[1] = zoomCam1;
	}
}

void updateCamera2()
{
    if (activeCam != 2) return;

    float pivotX = drone.position[0];
    float pivotY = drone.position[1] + followHeight;
    float pivotZ = drone.position[2];

    float yawRad   = mu.DegToRad(drone.rotation[1] + followYawOffsetDeg);
    float pitchRad = mu.DegToRad(followPitchOffsetDeg);

    float offsetX = followDistance * sinf(yawRad) * cosf(pitchRad);
    float offsetY = followDistance * sinf(pitchRad);
    float offsetZ = followDistance * cosf(yawRad) * cosf(pitchRad);

    cams[2].camPos[0] = pivotX - offsetX;
    cams[2].camPos[1] = pivotY + offsetY;
    cams[2].camPos[2] = pivotZ - offsetZ;

    cams[2].camTarget[0] = pivotX;
    cams[2].camTarget[1] = pivotY;
    cams[2].camTarget[2] = pivotZ;
}


bool checkAABBCollision(const float minA[3], const float maxA[3], const float minB[3], const float maxB[3]) {
    for (int i = 0; i < 3; i++) {
        if (maxA[i] < minB[i] || minA[i] > maxB[i]) {
            return false; 
        }
    }
    return true;
}

void updateFlyingObjects(float dt){
	for (auto &obj : flyingObjects)
	{
		// Move forward
		obj.position[0] += obj.direction[0] * obj.speed * dt;
		obj.position[1] += obj.direction[1] * obj.speed * dt;
		obj.position[2] += obj.direction[2] * obj.speed * dt;

		obj.speed *= 1.00001f;
		obj.rotationAngle += 40.0f * obj.rotationSpeed * dt;
        if (obj.rotationAngle > 360.0f) obj.rotationAngle -= 360.0f;

		// Respawn if out of visible region
		if (fabs(obj.position[0]) > 150 || fabs(obj.position[2]) > 150)
		{
			obj.position[0] = (rand() % 200 - 100);
			obj.position[1] = 10.0f + rand() % 30;
			obj.position[2] = (rand() % 200 - 100);

			float angle = (rand() % 360) * 3.14159f / 180.0f;
			obj.direction[0] = cos(angle);
			obj.direction[1] = 0.0f;
			obj.direction[2] = sin(angle);

			obj.speed = 4.0f;
			obj.rotationAngle = 0.0f;
		}
	}
}

void updatePackage(float dt) {
    if (!collisionPackage) return;

    float pivotX = drone.position[0];
    float pivotY = drone.position[1] - 1.5f;
    float pivotZ = drone.position[2];

    float yawRad   = mu.DegToRad(drone.rotation[1]);
    float pitchRad = mu.DegToRad(followPitchOffsetDeg);

    float offsetX = sinf(yawRad) * cosf(pitchRad);
    float offsetY = sinf(pitchRad);
    float offsetZ = cosf(yawRad) * cosf(pitchRad);

    package.position[0] = pivotX - offsetX;
    package.position[1] = pivotY + offsetY;
    package.position[2] = pivotZ - offsetZ;

	package.rotation[0]  = drone.rotation[0];
	package.rotation[1] = drone.rotation[1] ;
	package.rotation[2] = drone.rotation[2];
}


bool collision(){
	bool collisionDetected = false;
	for (Building &b : buildings) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, b.worldAABB.aabbmin, b.worldAABB.aabbmax)) {
			printf("COLLISION with BUILDING object!");
			fireworks = 1;
			iniParticles();
			collisionDetected = true;
		}
	}

	for (FlyingObject &obj : flyingObjects) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with FLYING object!");
			collisionDetected = true;
			flyingColision = true;
		}
	}

	for (LampPost &obj : lampPosts) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with LAMPPOST object!");
			collisionDetected = true;
		}
	}

	for (Tree &obj : trees) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with TREE object!");
			collisionDetected = true;
		}
	}

	if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, floorObj.worldAABB.aabbmin, floorObj.worldAABB.aabbmax)) {
		printf("COLLISION with FLOOR object!");
		collisionDetected = true;
	}

	if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, package.worldAABB.aabbmin, package.worldAABB.aabbmax)) {
		collisionPackage = true;
	}

	return collisionDetected;
}

void update(){
	static int prevMs = -1;
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	if (prevMs < 0)
		prevMs = nowMs;
	float dt = (nowMs - prevMs) / 1000.0f;
	prevMs = nowMs;

	static bool initDone = false;
	static float initTimer = 0.0f;

	if (!initDone) {
		initTimer += dt;
		if (initTimer < 0.2f) {
			return;
		} else {
			initDone = true;
		}
	}
	
	if(!pause && !gameOver) { 
		static bool collisionPushedBack = false;
		if (drone.mode == NORMAL) {
			if (collision()) {
				drone.mode = COLLISION;
				collisionPushedBack = false; // Reset
				animationCollision = 0.0f;

				drone.battery -= 100.0f / 5.0f;
    			if (drone.battery <= 0.0f){
					drone.battery = 0.0f;
					pause = true;
					gameOver = true;
				}
			} else {
				updateDrone(dt); // Normal movement
			}
		}
		else if (drone.mode == COLLISION) {

			if (flyingColision == true) {
				// Reset to initial pos
				restartDrone();
				flyingColision = false;
				drone.mode = NORMAL;
			} 
			else{
				if (!collisionPushedBack) {
					float pushBackDistance = collisionPackage ? 10.0f : 1.0f; //FIX
					
					drone.position[0] -= drone.direction[0] * pushBackDistance;
					drone.position[1] -= drone.direction[1] * pushBackDistance;
					drone.position[2] -= drone.direction[2] * pushBackDistance;

					// Freeze movement
					drone.direction[0] = 0.0f;
					drone.direction[1] = 0.0f;
					drone.direction[2] = 0.0f;

					collisionPushedBack = true;
				}

				animationCollision += dt;
				if (animationCollision >= animationCollisionDuration ) { 
					drone.mode = NORMAL;
				}
			}
		}
		manageBattery(dt);
		updateFlyingObjects(dt);
		updatePackage(dt);
	}
	updateCamera2();
    updateCameras();
}

void renderText(const std::string& textStr, const float position[2], const float color[3], float size) {
	TextCommand txt;
	txt.str = textStr;
		
	txt.position[0] = position[0];
	txt.position[1] = position[1];

	txt.size = size;
	txt.color[0] = color[0];
	txt.color[1] = color[1];
	txt.color[2] = color[2];
	txt.color[3] = color[3];
	txt.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

	renderer.renderText(txt);
}

void renderSim(void)
{

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderer.activateRenderMeshesShaderProg();

	renderer.setFog(gFogOn, gFogColor, gFogStart, gFogEnd);

	update();

	renderer.setTexUnit(0, 0);
	renderer.setTexUnit(1, 1);
	renderer.setTexUnit(2, 2);
	renderer.setTexUnit(3, 3);
	renderer.setTexUnit(4, 4);
	renderer.setTexUnit(5, 5);
	renderer.setTexUnit(6, 6);
	renderer.setTexUnit(7, 7);


	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);

	mu.lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
			  cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2], 0, 1, 0);

	// Directional light
	{
		float dirLightWorld[4] = {0.5f, -0.7f, 0.3f, 0.0f};
		float dirLightEye[4];
		mu.multMatrixPoint(gmu::VIEW, dirLightWorld, dirLightEye);
		float dirLightEye3[3] = {dirLightEye[0], dirLightEye[1], dirLightEye[2]};
		renderer.setDirectionalLight(dirLightEye3, dayMode ? dirLightColor : nightLightColor);
	}

	dataMesh data;

	//Headlight offsets
	float localHeadlightOffsets[2][4] = {
		{-0.5f, 0.0f, 1.0f, 1.0f}, // left headlight
		{0.5f, 0.0f, 1.0f, 1.0f}   // right headlight
	};

	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);
	mu.rotate(gmu::MODEL, drone.rotation[1], 0.0f, 1.0f, 0.0f);

	// For each headlight
	for (int i = 0; i < 2; i++)
	{
		float worldPos[4];
		mu.multMatrixPoint(gmu::MODEL, localHeadlightOffsets[i], worldPos);

		float eyePos[4];
		mu.multMatrixPoint(gmu::VIEW, worldPos, eyePos);

		droneHeadlights[i].Position[0] = eyePos[0];
		droneHeadlights[i].Position[1] = eyePos[1];
		droneHeadlights[i].Position[2] = eyePos[2];

		// Forward direction of drone
		float localDir[4] = {0.0f, 0.0f, 1.0f, 0.0f};
		float worldDir[4];
		mu.multMatrixPoint(gmu::MODEL, localDir, worldDir);

		float eyeDir[4];
		mu.multMatrixPoint(gmu::VIEW, worldDir, eyeDir);

		droneHeadlights[i].Direction[0] = eyeDir[0];
		droneHeadlights[i].Direction[1] = eyeDir[1];
		droneHeadlights[i].Direction[2] = eyeDir[2];

		droneHeadlights[i].Color[0] = 1.0f;
		droneHeadlights[i].Color[1] = 1.0f;
		droneHeadlights[i].Color[2] = 0.9f;
		droneHeadlights[i].atten.constant = 1.0f;
		droneHeadlights[i].atten.linear = 0.1f;
		droneHeadlights[i].atten.exp = 0.01f;
		droneHeadlights[i].Cutoff = cosf(20.0f * 3.14159f / 180.0f);
	}
	mu.popMatrix(gmu::MODEL);

	renderer.setDroneSpotLights(droneHeadlights, 2, spotlight_mode);

	// ----- RENDER FLYING OBJECTS -----
	for (auto &obj : flyingObjects)
	{
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, obj.position[0], obj.position[1], obj.position[2]);
		mu.rotate(gmu::MODEL, obj.rotationAngle, 0.0f, 1.0f, 0.0f);
		mu.scale(gmu::MODEL, 5.0f, 5.0f, 5.0f);

		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(obj.aabb, modelMatrix);
		obj.worldAABB = aabbBox;

		data.mesh = &allMeshes.cube;
		data.texMode = 1;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);

		mu.popMatrix(gmu::MODEL);
	}

	// --- Draw floor ---
	{	
		mu.pushMatrix(gmu::MODEL);
		mu.scale(gmu::MODEL, 250.0f, 0.1f, 200.0f);
		mu.rotate(gmu::MODEL, -90.0f, 1.0f, 0.0f, 0.0f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(floorObj.aabb, modelMatrix);
		floorObj.worldAABB = aabbBox;

		data.mesh = &allMeshes.quad;
		data.texMode = 10; // Multiple texturing
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	// --- Draw drone ---
	{
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);

		const float bodyWidth  = 1.0f;
		const float bodyHeight = 0.5f;
		const float bodyDepth  = 1.0f;

		// Center & rotate drone
		mu.translate(gmu::MODEL, bodyWidth * 0.5f, bodyHeight * 0.5f, bodyDepth * 0.5f);
		mu.rotate(gmu::MODEL, drone.rotation[0], 1.0f, 0.0f, 0.0f);
		mu.rotate(gmu::MODEL, drone.rotation[2], 0.0f, 0.0f, 1.0f);
		mu.rotate(gmu::MODEL, drone.rotation[1], 0.0f, 1.0f, 0.0f);
		mu.translate(gmu::MODEL, -bodyWidth * 0.5f, -bodyHeight * 0.5f, -bodyDepth * 0.5f);

		// Scale and render body
		mu.pushMatrix(gmu::MODEL);
		mu.scale(gmu::MODEL, bodyWidth, bodyHeight, bodyDepth);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		if (collisionPackage){
			drone.worldAABB = mergeAABBs( drone.worldAABB , package.worldAABB);
		} else {
			drone.worldAABB = updateGlobalAABB(drone.aabb, modelMatrix);
		}

		data.mesh = &allMeshes.cube;
		data.texMode = 4;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);

		// --- Drone motors ---
		const float motorRadius = 2.0f;
		const float motorHeight = 2.0f;
		const float motorYOffset = bodyHeight;

		struct Vec3 { float x, y, z; };
		Vec3 motorOffsets[4] = {
			{0.0f, motorYOffset, 0.0f},
			{bodyWidth, motorYOffset, 0.0f},
			{0.0f, motorYOffset, bodyDepth},
			{bodyWidth, motorYOffset, bodyDepth}
		};

		for (int i = 0; i < 4; i++) {
			mu.pushMatrix(gmu::MODEL);
			mu.translate(gmu::MODEL, motorOffsets[i].x, motorOffsets[i].y, motorOffsets[i].z);
			mu.scale(gmu::MODEL, motorRadius, motorHeight, motorRadius);
			mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
			mu.computeNormalMatrix3x3();

			data.mesh = &allMeshes.torus;
			data.texMode = 4;
			data.vm = mu.get(gmu::VIEW_MODEL);
			data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
			data.normal = mu.getNormalMatrix();
			renderer.renderMesh(data);

			mu.popMatrix(gmu::MODEL);
		}

		mu.popMatrix(gmu::MODEL);
	}

	// --- Draw garden ---
	{
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, gardenCenterX, 0.3f, gardenCenterZ);
		mu.scale(gmu::MODEL, gardenW, 1.0f, gardenD);
		mu.rotate(gmu::MODEL, -90.0f, 1.0f, 0.0f, 0.0f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();
		data.mesh = &allMeshes.quad;
		data.texMode = 5;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	// --- Draw trees ---
	for (Billboard &tree : billboards) {
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, tree.position[0], tree.position[1] + tree.scale[1] * 0.3, tree.position[2]);
        mu.scale(gmu::MODEL, tree.scale[0], tree.scale[1], tree.scale[2]);

		turnBillBoard(cams[2].camPos, tree.position);

        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(tree.aabb, modelMatrix);
		tree.worldAABB = aabbBox;

       	data.mesh = &allMeshes.quad;	
		data.texMode = tree.textureID;
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
    }

	// --- Draw buildings ---
	for (Building &b : buildings)
	{
		float h = buildingHeights[b.row][b.col];
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, b.position[0], 0, b.position[2]);

		mu.scale(gmu::MODEL, b.width, h, b.depth);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(b.aabb, modelMatrix);
		b.worldAABB = aabbBox;

		data.mesh = &allMeshes.cube;
		data.texMode = b.goal? 4 : 3;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();

		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	pointLights.clear();

	// --- Draw lamp post ---
	for (LampPost &lamp : lampPosts) {
        float lx = lamp.position[0];
        float ly = lamp.height;
        float lz = lamp.position[2];

        float worldPos[4] = {lx, ly, lz, 1.0f};
        float eyePos[4];
        mu.multMatrixPoint(gmu::VIEW, worldPos, eyePos);

        PointLight pl;
        pl.LocalPos[0] = eyePos[0];
        pl.LocalPos[1] = eyePos[1];
        pl.LocalPos[2] = eyePos[2];
        pl.Color[0] = 3.0f;
        pl.Color[1] = 2.7f;
        pl.Color[2] = 2.1f;
        pl.atten.constant = 1.0f;
        pl.atten.linear   = 0.02f;
        pl.atten.exp      = 0.02f;
        pointLights.push_back(pl);

        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, lx, 0, lz);
        mu.scale(gmu::MODEL, 0.25f, lamp.height, 0.25f);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(lamp.aabb, modelMatrix);
		lamp.worldAABB = aabbBox;

        data.mesh = &allMeshes.cube;
        data.texMode = 4;
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
    }
	renderer.setLampLights(pointLights, lampsOn);

	// --- Draw package ---
	{ 
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, package.position[0], package.position[1], package.position[2]);

		mu.translate(gmu::MODEL, 0.5f, 0.5f, 1.0f);

		mu.rotate(gmu::MODEL, package.rotation[1], 0.0f, 1.0f, 0.0f);
		mu.rotate(gmu::MODEL, package.rotation[0], 1.0f, 0.0f, 0.0f);
		mu.rotate(gmu::MODEL, package.rotation[2], 0.0f, 0.0f, 1.0f);
		mu.translate(gmu::MODEL, -0.5f, -0.5f, -1.0f);

		mu.scale(gmu::MODEL, 1.0f, 1.0f, 2.0f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(package.aabb, modelMatrix);
		package.worldAABB = aabbBox;

		data.mesh = &allMeshes.cube;
		data.texMode = 4;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();

		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	// ----- RENDER PARTICLES -----
	if(fireworks){

		updateParticles();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glEnable(GL_DEPTH_TEST);


		dataMesh data;
		data.mesh = &allMeshes.sphere[1];
		data.texMode = 9;


		for (int i = 0; i < MAX_PARTICULAS; ++i) {
			if (particles[i].life > 0.0f) {
				
				mu.pushMatrix(gmu::MODEL);
				mu.translate(gmu::MODEL, particles[i].x, particles[i].y, particles[i].z);
				mu.scale(gmu::MODEL, 0.05f, 0.05f, 0.05f); 

				mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
				mu.computeNormalMatrix3x3();
				data.vm = mu.get(gmu::VIEW_MODEL);
				data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
				data.normal = mu.getNormalMatrix();

				renderer.renderMesh(data);

				mu.popMatrix(gmu::MODEL);
			} else {
				dead_num_particles++;
			}
		}

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		if (dead_num_particles == MAX_PARTICULAS) {
			fireworks = 0;
			dead_num_particles = 0;
			printf("All particles dead\n");
		}

	}


	// --- Draw bulb ---
	glDepthMask(GL_FALSE);
	for (LampPost &lamp : lampPosts) {
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, lamp.position[0], lamp.height + 1.5, lamp.position[2]);
        mu.scale(gmu::MODEL, 1.5f, 1.5f, 1.5f);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        data.mesh = &allMeshes.sphere[0];
        data.texMode = 2; // Transparency
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
	}
	glDepthMask(GL_TRUE);

	std::array<float, 2> position;
	std::array<float, 4> color;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	// Save current matrices
	mu.pushMatrix(gmu::MODEL);
	mu.pushMatrix(gmu::VIEW);
	mu.pushMatrix(gmu::PROJECTION);

	// Setup ortho for HUD
	mu.loadIdentity(gmu::MODEL);
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::PROJECTION);
	mu.ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, 
			m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);

	if (!pause && !gameOver) {
		// Render energy
		int maxEnergy = 5;
		int currentEnergy = static_cast<int>((drone.battery / 100.0f) * maxEnergy + 0.5f);
		float startX = 50, startY = 50, spacing = 40;
		for (int i = 0; i < maxEnergy; ++i) {
			std::string str = (i < currentEnergy) ? "H" : " ";
			position = { startX + i * spacing, startY };
			color = { 1.0f, 1.0f, 0.0f, 1.0f };
			renderText(str, position.data(), color.data(), 1.0f);
		}

		// Energy percentage text
		char energyStr[32];
		sprintf(energyStr, "%.0f%%", drone.battery);
		position  = { startX + maxEnergy * spacing + 40, startY + 80 };
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderText(energyStr, position.data(), color.data(), 0.5f);

		// Render points
		std::string pointsStr = "Points: 120";
		position = { static_cast<float>(m_viewport[2]) - 300, 120 };
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderText(pointsStr, position.data(), color.data(), 0.5f);

	}

	if (gameOver) {
		std::string gameOverStr = "GAME OVER";
		position = { m_viewport[2] - m_viewport[2] * 0.80f,  m_viewport[3] - m_viewport[3] * 0.6f };
		color = { 1.0f, 0.0f, 0.0f, 1.0f };
		renderText(gameOverStr, position.data(), color.data(), 2.0f);
	}

	if (pause && !gameOver) {
		std::string pauseStr = "PAUSE";
		position = { m_viewport[2] - m_viewport[2] * 0.60f,  m_viewport[3] - m_viewport[3] * 0.6f };
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderText(pauseStr, position.data(), color.data(), 2.0f);

		std::string resumeStr = "Press P to resume";
		position = { m_viewport[2] - m_viewport[2] * 0.65f,   m_viewport[3] - m_viewport[3] * 0.5f };
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderText(resumeStr, position.data(), color.data(), 1.0f);

		std::string restartStr = "Press R to restart";
		position = { m_viewport[2] - m_viewport[2] * 0.65f,   m_viewport[3] - m_viewport[3] * 0.6f };
		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderText(restartStr, position.data(), color.data(), 1.0f);
	}

	// Restore original matrices
	mu.popMatrix(gmu::PROJECTION);
	mu.popMatrix(gmu::VIEW);
	mu.popMatrix(gmu::MODEL);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glutSwapBuffers();
}



// ------------------------------------------------------------
//
// Events from the Keyboard
//

void keyUp(unsigned char key, int x, int y)
{
	keyStates[key] = false;
}

void processKeys(unsigned char key, int xx, int yy)
{
	keyStates[key] = true;

	switch (key)
	{

	case 27:
		glutLeaveMainLoop();
		break;

	case 'h':
		spotlight_mode = !spotlight_mode;
		printf("Spot light %s\n", spotlight_mode ? "ON" : "OFF");
		break;

	case 'c': // toggle lamp posts
		lampsOn = !lampsOn;
		printf("Lamp posts %s\n", lampsOn ? "ON" : "OFF");
		break;

	case 'n':
		dayMode = !dayMode;
		printf("Ambient Light %s\n", spotlight_mode ? "ON" : "OFF");
		break;
	
	case 'f':
	case 'F':
		gFogOn = !gFogOn;
		printf("Fog %s\n", gFogOn ? "ON" : "OFF");
		break;
	case 'p':
		printf("paused game!\n");
		pause = !pause;
		break;
	case 'r':
		printf("restart game!\n");
		restart = true;
		pause = false;
		gameOver = false;
		break;
	case '1':
		activeCam = 0;
		break;
	case '2':
		activeCam = 1;
		break;
	case '3':
		activeCam = 2;
		break;
	case '4':
		activeCam = 3;
		break;
	
	}
}

void processSpecialDown(int key, int x, int y)
{
	spKeys[key] = true;
}
void processSpecialUp(int key, int x, int y)
{
	spKeys[key] = false;
}

// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	if (state == GLUT_DOWN)
	{
		startX = prevX = xx;
		startY = prevY = yy;

		if (button == GLUT_LEFT_BUTTON)
		{
			tracking = 1; // orbit
		}
		else if (button == GLUT_RIGHT_BUTTON)
		{
			tracking = 2; // zoom
			startFollowDistance = followDistance;
		}
	}
	else if (state == GLUT_UP)
	{
		tracking = 0;
	}
}

void processMouseMotion(int xx, int yy) {
    int deltaX = xx - startX;
    int deltaY = yy - startY;

    if (tracking == 1) { // rotate camera
        followYawOffsetDeg   += deltaX * 0.3f;
        followPitchOffsetDeg += deltaY * 0.3f;

        if (followPitchOffsetDeg > 85.0f) followPitchOffsetDeg = 85.0f;
        if (followPitchOffsetDeg < -85.0f) followPitchOffsetDeg = -85.0f;

        startX = xx; // update last mouse pos
        startY = yy;
    }
    else if (tracking == 2) { // zoom
		if (activeCam == 2) {
			followDistance += deltaY * 0.1f;
			if (followDistance < 1.0f) followDistance = 1.0f;
		}
		else if (activeCam == 0) { // top perspective
			zoomCam1 += deltaY * 0.5f;
			if (zoomCam1 < 10.0f) zoomCam1 = 10.0f;
		}
		else if (activeCam == 1) { // top orthographic
			zoomCam2 += deltaY * 0.5f;
			if (zoomCam2 < 5.0f) zoomCam2 = 5.0f;
		}

        startX = xx;
        startY = yy;
    }
}

void mouseWheel(int wheel, int direction, int x, int y)
{
    float delta = (float)direction;

    if (activeCam == 2) { // drone follow cam
        followDistance -= delta * 0.5f;
        if (followDistance < 1.0f) followDistance = 1.0f;
    }
    else if (activeCam == 0) { // top perspective
        zoomCam1 -= delta * 5.0f;
        if (zoomCam1 < 10.0f) zoomCam1 = 10.0f;
    }
    else if (activeCam == 1) { // top orthographic
        zoomCam2 -= delta * 2.0f;
        if (zoomCam2 < 5.0f) zoomCam2 = 5.0f;
    }
	
}

//
// Scene building with basic geometry
//

void buildScene()
{
	// Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/pavement.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");
	renderer.TexObjArray.texture2D_Loader("assets/Bricks097.tga");
	renderer.TexObjArray.texture2D_Loader("assets/metal.tga");
	renderer.TexObjArray.texture2D_Loader("assets/grass.tga");
	renderer.TexObjArray.texture2D_Loader("assets/road.tga");
	renderer.TexObjArray.texture2D_Loader("assets/tree.tga");

	// Scene geometry with triangle meshes
	MyMesh amesh;

	float amb[] = {0.2f, 0.15f, 0.1f, 1.0f};
	float diff[] = {0.8f, 0.6f, 0.4f, 1.0f};
	float spec[] = {0.8f, 0.8f, 0.8f, 1.0f};

	float amb1[] = {0.3f, 0.0f, 0.0f, 1.0f};
	float diff1[] = {0.8f, 0.1f, 0.1f, 1.0f};
	float spec1[] = {0.3f, 0.3f, 0.3f, 1.0f};

	float ambBulb[] = {0.2f, 0.2f, 0.0f, 0.3f};
	float diffBulb[] = {0.8f, 0.8f, 0.2f, 0.3f};
	float specBulb[] = {0.5f, 0.5f, 0.3f, 0.3f};
	float emisBulb[] = {1.0f, 1.0f, 0.2f, 0.3f};

	float ambMotor[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float diffMotor[] = {0.05f, 0.05f, 0.05f, 1.0f};
	float specMotor[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float emisMotor[] = {0.0f, 0.0f, 0.0f, 1.0f};

	float ambFireworks[]  = { 0.3f, 0.1f, 0.0f, 1.0f };   
	float diffFireworks[] = { 0.9f, 0.3f, 0.0f, 1.0f }; 
	float specFireworks[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	float shininessFireworks   = 10.0f;

	float emissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float shininess = 100.0f;
	int texcount = 0;

	// Quad
	allMeshes.quad = createQuad(1, 1);
	memcpy(allMeshes.quad.mat.ambient, amb, 4 * sizeof(float));
	memcpy(allMeshes.quad.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(allMeshes.quad.mat.specular, spec, 4 * sizeof(float));
	memcpy(allMeshes.quad.mat.emissive, emissive, 4 * sizeof(float));
	allMeshes.quad.mat.shininess = shininess;
	allMeshes.quad.mat.texCount = texcount;

	// Cube
	allMeshes.cube = createCube();
	memcpy(allMeshes.cube.mat.ambient, amb, 4 * sizeof(float));
	memcpy(allMeshes.cube.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(allMeshes.cube.mat.specular, spec, 4 * sizeof(float));
	memcpy(allMeshes.cube.mat.emissive, emissive, 4 * sizeof(float));
	allMeshes.cube.mat.shininess = shininess;
	allMeshes.cube.mat.texCount = texcount;

	// Sphere
	allMeshes.sphere[0] = createSphere(1.0f, 16);
	memcpy(allMeshes.sphere[0].mat.ambient, ambBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere[0].mat.diffuse, diffBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere[0].mat.specular, specBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere[0].mat.emissive, emisBulb, 4 * sizeof(float));
	allMeshes.sphere[0].mat.shininess = shininess;
	allMeshes.sphere[0].mat.texCount = texcount;

	allMeshes.sphere[1] = createSphere(1.0f, 20);
	memcpy(allMeshes.sphere[1].mat.ambient, ambFireworks, 4 * sizeof(float));
	memcpy(allMeshes.sphere[1].mat.diffuse, diffFireworks, 4 * sizeof(float));
	memcpy(allMeshes.sphere[1].mat.specular, specFireworks, 4 * sizeof(float));
	memcpy(allMeshes.sphere[1].mat.emissive, emissive, 4 * sizeof(float));
	allMeshes.sphere[1].mat.shininess = shininessFireworks;
	allMeshes.sphere[1].mat.texCount = texcount;

	// Cone
	allMeshes.cone = createCone(2.0f, 1.0f, 20);
	memcpy(allMeshes.cone.mat.ambient, amb, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.specular, spec, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.emissive, emissive, 4 * sizeof(float));
	allMeshes.cone.mat.shininess = shininess;
	allMeshes.cone.mat.texCount = texcount;

	// Torus
	allMeshes.torus = createTorus(0.1f, 0.2f, 16, 12);
	memcpy(allMeshes.torus.mat.ambient, ambMotor, 4 * sizeof(float));
	memcpy(allMeshes.torus.mat.diffuse, diffMotor, 4 * sizeof(float));
	memcpy(allMeshes.torus.mat.specular, specMotor, 4 * sizeof(float));
	memcpy(allMeshes.torus.mat.emissive, emisMotor, 4 * sizeof(float));
	allMeshes.torus.mat.shininess = shininess;
	allMeshes.torus.mat.texCount = texcount;


	// --- INITIALIZE FLYING OBJECTS ---
	for (int i = 0; i < 10; i++)
	{
		FlyingObject obj;

		obj.position[0] = (rand() % 200 - 100);
		obj.position[1] = 10.0f + rand() % 30;
		obj.position[2] = (rand() % 200 - 100);

		float angle = (rand() % 360) * 3.14159f / 180.0f;
		obj.direction[0] = cos(angle);
		obj.direction[1] = 0.0f;
		obj.direction[2] = sin(angle);

		obj.speed = 4.0f;
		obj.rotationAngle = 0.0f;
		obj.rotationSpeed = 0.01f + (rand() % 5);
		obj.meshID = 2 + (i % 2);
		obj.active = true;
		obj.aabb = allMeshes.cube.aabb;
		flyingObjects.push_back(obj);
	}

	// --- INITIALIZE BUILDINGS ---
	buildings.clear();
	for (int r = 0; r < rows; ++r)
	{
		for (int c = 0; c < cols; ++c)
		{
			float x = offsetX + c * (buildingW + gap);
			float z = offsetZ + r * (buildingD + gap);

			bool isStreet = false;
			// create vertical street down the middle
			if (c == cols / 2)
				isStreet = true;
			// create horizontal street across middle
			if (r == rows / 2)
				isStreet = true;

			// carve out the central garden area
			int gardenRowStart = rows / 2 - gardenSizeRows / 2;
			int gardenRowEnd = gardenRowStart + gardenSizeRows - 1;
			int gardenColStart = cols / 2 - gardenSizeCols / 2;
			int gardenColEnd = gardenColStart + gardenSizeCols - 1;
			bool isGardenCell = (r >= gardenRowStart && r <= gardenRowEnd && c >= gardenColStart && c <= gardenColEnd);

			if (!isStreet && !isGardenCell)
			{
				Building b;
				b.position[0] = x;
				b.position[2] = z;
				b.width = buildingW;
				b.depth = buildingD;
				b.row = r;
				b.col = c;
				b.aabb = allMeshes.cube.aabb;

				buildings.push_back(b);

				buildingHeights[r][c] = std::max((rand() % 20) + 8.0f, 10.0f);
			}
			else
			{
				// leave empty
				buildingHeights[r][c] = 0.0f;
			}
		}
	}

	// --- INITIALIZE LAMP POSTS ---
	float lampRadius = std::max(gardenW, gardenD) / 2.0f; 
	int lampCount = LAMP_POST_NUMBER;
    for (int i = 0; i < lampCount; ++i) {
        float ang = (2.0f * 3.14159f) * i / lampCount;
        float lx = gardenCenterX + lampRadius * cos(ang);
        float lz = gardenCenterZ + lampRadius * sin(ang);
        float ly = lampHeight;

        LampPost lamp;
        lamp.position[0] = lx;
        lamp.position[1] = 0.0f;
        lamp.position[2] = lz;
        lamp.height = ly;
        lamp.aabb = allMeshes.cube.aabb;

        lampPosts.push_back(lamp);
    }

	// --- INITIALIZE TREES ---
	for (int t = 0; t < 6; ++t) {
        float ang = (2.0f * 3.14159f) * t / 6.0f;
        float rx = gardenCenterX + (gardenW / 2.2f - 4.0f) * cos(ang);
        float rz = gardenCenterZ + (gardenD / 2.2f - 4.0f) * sin(ang);

        Billboard tree;
        tree.position[0] = rx;
        tree.position[1] = 0.0f;
        tree.position[2] = rz;

        tree.scale[0] = 20.0f;
        tree.scale[1] = 20.0f;
        tree.scale[2] = 20.0f;
		tree.textureID = 7;
        tree.aabb = allMeshes.quad.aabb;

       	billboards.push_back(tree);
    }

	// --- INITIALIZE DRONE ---
	{
		drone.position[0] = 20.0f;
		drone.position[1] = 20.0f;
		drone.position[2] = -20.0f;

		drone.direction[0] = 0.0f;
		drone.direction[1] = 0.0f;
		drone.direction[2] = -1.0f;
		drone.aabb = allMeshes.cube.aabb;

		drone.battery = 100.0f;
		drone.points = 0;
	}

	// --- INITIALIZE PACKAGE ---
	{
		randomPackagePos();
		package.aabb = allMeshes.cube.aabb;
	}

	floorObj.aabb = allMeshes.quad.aabb;

	cams[0].camPos[1] = 200.0;
	cams[0].camPos[0] = 0.0;
	cams[0].camPos[2] = 0.33;
	// top ortho
	cams[1].camPos[1] = 200.0;
	cams[1].camPos[2] = 0.33;
	cams[1].type = 1;
	
	fontLoaded = renderer.truetypeInit(fontPathFile);
	if (!fontLoaded)
		cerr << "Fonts not loaded\n";
	else 
		cerr << "Fonts loaded\n";

}

// ------------------------------------------------------------
//
// Main function
//

int main(int argc, char **argv)
{

	//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);

	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);

	//  Callback Registration
	glutDisplayFunc(renderSim);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	glutTimerFunc(0, refresh, 0);

	//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutKeyboardUpFunc(keyUp);

	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc(mouseWheel);

	glutSpecialFunc(processSpecialDown);
	glutSpecialUpFunc(processSpecialUp);

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

	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	buildScene();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	//  GLUT main loop
	glutMainLoop();

	return (0);
}
