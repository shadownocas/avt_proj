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
#include <cmath>	 // sinf, cosf, atan2f
#include <algorithm> // if you use std::clamp elsewhere

using namespace std;

#define CAPTION "AVT 2025 Welcome Demo"
#define LAMP_POST_NUMBER 6
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

// File with the font
const string fontPathFile = "fonts/arial.ttf";

// Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

// Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;

// Camera Position
// Follow-camera mouse offsets (degrees)
float followYawOffsetDeg = 0.f;
float followPitchOffsetDeg = 15.f; // slight downward tilt by default
float minPitchDeg = -10.0f, maxPitchDeg = 85.0f;
// mouse sensitivity & state
float MOUSE_SENS_YAW = 0.25f;	// deg per pixel
float MOUSE_SENS_PITCH = 0.25f; // deg per pixel
float startYawDeg = 0.f, startPitchDeg = 0.f;
float startFollowDistance = 20.f;
// Zoom sensitivity (world units per pixel of RMB drag)
float ZOOM_SENS = 0.05f;

float sYawDeg = 0.0f, sPitchDeg = 15.0f, sDist = 15.0f; // init to your defaults
int prevX = 0, prevY = 0;

// --- Drone flight state (Task 4)
float yawDeg = 0.0f;   // heading angle (deg)
float pitchDeg = 0.0f; // nose up/down (+ = back)
float rollDeg = 0.0f;  // bank left/right (+ = left)

// --- Forward motion (for UP/DOWN arrows)
float vFwd = 0.0f;				   // forward speed (units/s)
const float FWD_MAX_SPEED = 16.0f; // cap (tune)
const float FWD_GAIN = 3.0f;	   // 1/s — ramp rate to target
const float PITCH_VIS_MAX = 10.0f; // deg of visual nose tilt at full speed

float velX = 0.0f, velY = 0.0f, velZ = 0.0f; // world velocities

// Controls & physics
const float maxTiltDeg = 25.0f;	  // clamp for pitch/roll
const float tiltStepDeg = 1.5f;	  // degrees added per frame while arrow held
const float tiltReturnDeg = 1.0f; // auto-level speed per frame when released
const float accelGain = 0.02f;	  // horizontal accel per frame from tilt
const float horizDrag = 0.02f;	  // decay per frame (0..1)
const float vertAccel = 0.01f;	  // W/S vertical accel per frame
const float vertDrag = 0.01f;	  // vertical decay per frame
const float maxHorizSpeed = 0.6f; // clamp XY speed
const float maxVertSpeed = 0.5f;  // clamp Y speed

// Special (arrow) key states
bool spKeys[256] = {false}; // GLUT special keys (UP/DOWN/LEFT/RIGHT)

bool keyStates[256] = {false};

struct Drone
{
	float position[3] = {20.0f, 20.0f, -20.0f}; // xyz
	float direction[3] = {0.0f, 0.0f, -1.0f};	// pointing along -Z initially
	float speed;
	float yaw;
	AABB aabb;
	AABB worldAABB;
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

std::vector<PointLight> pointLights;

std::vector<FlyingObject> flyingObjects;

std::vector<LampPost> lampPosts;

std::vector<Tree> trees;


Drone drone;
float droneSpeed = 0.2f;	  // units per frame
float followDistance = 15.0f; // camera distance behind drone
float followHeight = 5.0f;	  // camera height above drone
float droneRotSpeed = 1.5f;	  // degrees per frame

struct Camera
{
	float camPos[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // camera 4 for orbit view of drone
	float camTarget[3] = {0.0f, 0.0f, 0.0f};
	int type = 0; // 0:perspective, 1:orthographic
};

Camera cams[4];

int activeCam = 0;

bool lampsOn = true; // all lamps initially on

std::vector<Lamp> lampPositions;

MeshCollection allMeshes;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 57.0f, beta = 18.0f;
float r = 45.0f;

// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];

bool dayMode = true;						  // Day/night toggle
float dirLightDir[3] = {-0.5f, -1.0f, -0.3f}; // Direction of sunlight
float dirLightColor[3] = {1.0f, 1.0f, 0.9f};  // Day color
// float nightLightColor[3] = { 0.1f, 0.1f, 0.2f }; // Night ambient color
float nightLightColor[3] = {0.0f, 0.0f, 0.0f};

float lightPos[4] = {4.0f, 20.0f, 2.0f, 1.0f};
// float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

// Spotlight
bool spotlight_mode = false;
float coneDir[4] = {0.0f, -0.0f, -1.0f, 0.0f};

SpotLight droneHeadlights[2]; // two headlights

int rows = 7; // number of rows
int cols = 7; // number of columns
float gap = 20.0f;
float offsetX = -((cols - 1) * (10.0f + gap)) / 2.0f; // center grid 10 = buildingWidth
float offsetZ = -((rows - 1) * (10.0f + gap)) / 2.0f; // 10 = buildingDepth

int numLamps = std::min(rows, cols);
float lampHeight = 10.0f;
float lampOffset = (10.0f + gap) / 2.0f + 2.0f; // push them to the side of diagonal 10 = buildingWidth

bool fontLoaded = false;

struct Building
{
	float x, z;
	float width, depth;
	int row, col; // indices no grid
	AABB aabb;
	AABB worldAABB;
};

std::vector<Building> buildings;
std::vector<std::vector<float>> buildingHeights(rows, std::vector<float>(cols));

float buildingW = 10.0f;
float buildingD = 10.0f;
float streetWidth = 30.0f; // largura da rua
float sidewalk = 2.0f;	   // passeios

// central garden define: ocupa N x M células no centro
int gardenSizeRows = 3; // 2x2 cells
int gardenSizeCols = 3;

float gardenHalfW = (gardenSizeCols * (buildingW + gap) - gap) / 2.0f;
float gardenHalfD = (gardenSizeRows * (buildingD + gap) - gap) / 2.0f;

float gardenCenterX = 6.0f; // because grid centered with offsetX/offsetZ
float gardenCenterZ = 5.0f;
float gardenW = gardenHalfW * 2.0f;
float gardenD = gardenHalfD * 2.0f;


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

void refresh(int value) // faz RENDER
{
	glutPostRedisplay();
	glutTimerFunc(1000 / 60, refresh, 0);
}

void animate()
{	// UPDATE das posicoes e no render desenha!
	/* vel = theta * ...;
	pos += vel * deltaT */
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h)
{
	float ratio;
	// Prevent a divide by zero, when window is too short
	if (h == 0)
		h = 1;

	WinX = w;
	WinY = h;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	mu.loadIdentity(gmu::PROJECTION);
	if (activeCam == 0)
	{
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
		printf("entra change size prespectiev");
	}
	else
	{
		mu.ortho(0, w - 1, 0, h - 1, -1, 1);
	}
}

AABB updateGlobalAABB(AABB result, float* modelMatrix) {
    float globalMin[3] = { 100000000000, 100000000000, 100000000000 };
    float globalMax[3] = { -100000000000, -100000000000, -100000000000 };
	AABB obj = result; 
    for (int i = 0; i < 8; i++) {
        float* c = obj.corners[i];

        // Matrix × Vector (column-major order)
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

        // Update global min
        globalMin[0] = std::min(globalMin[0], x);
        globalMin[1] = std::min(globalMin[1], y);
        globalMin[2] = std::min(globalMin[2], z);

        // Update global max
        globalMax[0] = std::max(globalMax[0], x);
        globalMax[1] = std::max(globalMax[1], y);
        globalMax[2] = std::max(globalMax[2], z);
    }

    // Save results
    obj.aabbmin[0] = globalMin[0];
    obj.aabbmin[1] = globalMin[1];
    obj.aabbmin[2] = globalMin[2];

    obj.aabbmax[0] = globalMax[0];
    obj.aabbmax[1] = globalMax[1];
    obj.aabbmax[2] = globalMax[2];
	//printf("aabbUPDATEDDD min x: %f y: %f z: %f\n", obj.aabbmin[0], obj.aabbmin[1], obj.aabbmin[2]);

    return obj;
}

// ------------------------------------------------------------
//
// Render stufff
//

inline float clampf(float v, float lo, float hi)
{
	return (v < lo) ? lo : (v > hi) ? hi
									: v;
}

void updateDrone(float dt)
{
	const float D2R = 3.1415926f / 180.0f; //FIX

	// Tunables (per second)
	const float YAW_SPEED_DEG = 60.0f; // A/D yaw rate
	const float VERT_ACCEL = 5.0f;	   // W/S vertical accel (units/s^2)
	const float VERT_DRAG_S = 1.2f;	   // vertical drag (1/s)
	const float MAX_VSPEED = 20.0f;	   // max vertical speed (units/s)

	// A/D — yaw only (rotate heading)
	if (keyStates['a'])
		yawDeg -= YAW_SPEED_DEG * dt;
	if (keyStates['d'])
		yawDeg += YAW_SPEED_DEG * dt;

	drone.yaw = yawDeg;

	// update forward dir (useful for cameras/lights)
	float yawRad = yawDeg * D2R;
	drone.direction[0] = std::sin(yawRad);
	drone.direction[1] = 0.0f;
	drone.direction[2] = -std::cos(yawRad);

	// --- Up/Down arrows: forward/back target speed with smooth ramp ---
	float targetFwd = 0.0f;
	if (spKeys[GLUT_KEY_UP])
		targetFwd = +FWD_MAX_SPEED; // move forward
	else if (spKeys[GLUT_KEY_DOWN])
		targetFwd = -FWD_MAX_SPEED; // move backward

	// Ease current forward speed toward target (Option B style)
	vFwd += (targetFwd - vFwd) * FWD_GAIN * dt;

	// Map forward speed to world velocity using current yaw
	float sinY = drone.direction[0];
	float cosY = -drone.direction[2];
	velX = vFwd * sinY;
	velZ = vFwd * cosY;

	// Integrate horizontal motion
	drone.position[0] += velX * dt;
	drone.position[2] += velZ * dt;

	// --- Visual nose tilt proportional to forward speed ---
	float pitchTarget = +PITCH_VIS_MAX * (vFwd / FWD_MAX_SPEED);  //tilt direction the same as movement direction
	pitchDeg += (pitchTarget - pitchDeg) * 6.0f * dt; // smooth the tilt

	// W/S target vertical speed (smooth ramp) ---
	const float THROTTLE_GAIN = 3.0f; // 1/s — higher = snappier
	float targetVY = 0.0f;
	if (keyStates['w'])
		targetVY = +MAX_VSPEED; // rise toward +cap
	else if (keyStates['s'])
		targetVY = -MAX_VSPEED; // fall toward -cap

	// ease current speed toward target
	velY += (targetVY - velY) * THROTTLE_GAIN * dt;

	// optional tiny bleed so it doesn't hover forever when you release
	velY *= std::exp(-0.2f * dt);

	// safety clamp (should rarely do anything because target==cap)
	velY = clampf(velY, -MAX_VSPEED, MAX_VSPEED);

	// integrate position
	drone.position[1] += velY * dt;

	// keep above ground (optional)
	if (drone.position[1] < 0.5f)
	{
		drone.position[1] = 0.5f;
		velY = 0.0f;
	}
}

void updateCameras(){
	float ratio = (float)WinX / (float)WinY;

	mu.loadIdentity(gmu::PROJECTION);

	if (activeCam == 0 || activeCam == 2) {
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	} else if (activeCam == 1 ) {
		float orthoSize = 60.0f;
		mu.ortho(-orthoSize * ratio, orthoSize * ratio, -orthoSize, orthoSize, -500.0f, 500.0f);
	}
}

void updateCamera2() //dt needed??
{
    if (activeCam != 2) return;

    float pivotX = drone.position[0];
    float pivotY = drone.position[1] + followHeight;
    float pivotZ = drone.position[2];

    // Convert to radians consistently
    float yawRad   = mu.DegToRad(drone.yaw + followYawOffsetDeg);
    float pitchRad = mu.DegToRad(followPitchOffsetDeg);

    float targetX = pivotX - followDistance * sin(yawRad) * cos(pitchRad);
    float targetY = pivotY + followDistance * sin(pitchRad);
    float targetZ = pivotZ - followDistance * cos(yawRad) * cos(pitchRad);

    cams[2].camPos[0] = targetX;
    cams[2].camPos[1] = targetY;
    cams[2].camPos[2] = targetZ;

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
    return true; //collsionnn
}

void updateFlyingObjects(){
	for (auto &obj : flyingObjects)
	{
		// Move forward
		obj.position[0] += obj.direction[0] * obj.speed;
		obj.position[1] += obj.direction[1] * obj.speed;
		obj.position[2] += obj.direction[2] * obj.speed;

		// Increase speed slightly with play time
		obj.speed *= 1.0001f;

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

			obj.speed = 0.05f + (rand() % 10) * 0.01f;
			obj.rotationAngle = 0.0f;
		}
	}
}

void update(){ //UPDATE das posicoes e no render desenha!
	 // Update drone movement
	 // delta time (seconds)
	static int prevMs = -1;
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	if (prevMs < 0)
		prevMs = nowMs;
	float dt = (nowMs - prevMs) / 1000.0f;
	prevMs = nowMs;
	if (dt < 0.001f)
		dt = 0.001f;
	if (dt > 0.033f)
		dt = 0.033f;
	
	for (Building &b : buildings) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, b.worldAABB.aabbmin, b.worldAABB.aabbmax)) {
			printf("COLLISION with BUILDING object!");
		}
	}

	for (FlyingObject &obj : flyingObjects) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with FLYING object!");
		}
	}

	for (LampPost &obj : lampPosts) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with LAMPPOST object!");
		}
	}

	for (Tree &obj : trees) {
		if (checkAABBCollision(drone.worldAABB.aabbmin, drone.worldAABB.aabbmax, obj.worldAABB.aabbmin, obj.worldAABB.aabbmax)) {
			printf("COLLISION with TREE object!");
		}
	}

    updateDrone(dt);
	updateFlyingObjects();
	updateCamera2();
    updateCameras();
}

void renderSim(void)
{

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination

	update();

	renderer.setTexUnit(0, 0);
	renderer.setTexUnit(1, 1);
	renderer.setTexUnit(2, 2);
	renderer.setTexUnit(3, 3);
	renderer.setTexUnit(4, 4);
	renderer.setTexUnit(5, 5);
	renderer.setTexUnit(6, 6);

	// --- Set camera/view ---
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);

	// set the camera using a function similar to gluLookAt
	mu.lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
			  cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2], 0, 1, 0);

	// Directional light
	float dirLightWorld[4] = {0.5f, -0.7f, 0.3f, 0.0f};
	float dirLightEye[4];
	mu.multMatrixPoint(gmu::VIEW, dirLightWorld, dirLightEye);
	float dirLightEye3[3] = {dirLightEye[0], dirLightEye[1], dirLightEye[2]};
	renderer.setDirectionalLight(dirLightEye3, dayMode ? dirLightColor : nightLightColor);

	dataMesh data;

	// --- Headlight offsets in LOCAL space of the cube ---
	float localHeadlightOffsets[2][4] = {
		{-0.5f, 0.0f, 1.0f, 1.0f}, // left headlight
		{0.5f, 0.0f, 1.0f, 1.0f}   // right headlight
	};

	// Compute drone's rotation matrix
	float angle_Y = atan2(drone.direction[0], -drone.direction[2]);
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);
	mu.rotate(gmu::MODEL, angle_Y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);

	// For each headlight
	for (int i = 0; i < 2; i++)
	{
		float worldPos[4];
		mu.multMatrixPoint(gmu::MODEL, localHeadlightOffsets[i], worldPos);

		// Convert to eye space
		float eyePos[4];
		mu.multMatrixPoint(gmu::VIEW, worldPos, eyePos);

		droneHeadlights[i].Position[0] = eyePos[0];
		droneHeadlights[i].Position[1] = eyePos[1];
		droneHeadlights[i].Position[2] = eyePos[2];

		// Forward direction of drone (local Z axis)
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
		mu.scale(gmu::MODEL, 5.0f, 5.0f, 5.0f);
		mu.rotate(gmu::MODEL, obj.rotationAngle, 0.0f, 1.0f, 0.0f);

		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(obj.aabb, modelMatrix); //WORLD SPACEE
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
	mu.pushMatrix(gmu::MODEL);
	mu.scale(gmu::MODEL, 250.0f, 0.1f, 200.0f);
	mu.rotate(gmu::MODEL, -90.0f, 1.0f, 0.0f, 0.0f);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	data.mesh = &allMeshes.quad;
	data.texMode = 2;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	renderer.renderMesh(data);
	mu.popMatrix(gmu::MODEL);

	// --- Draw drone (green cube body + 4 white motors) ---
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);
	const float bodyX = 1.6f, bodyY = 0.25f, bodyZ = 1.6f;
	mu.translate(gmu::MODEL, bodyX * 0.5f, bodyY * 0.5f, bodyZ * 0.5f);	   // to center
	mu.rotate(gmu::MODEL, yawDeg, 0.0f, 1.0f, 0.0f);					   // yaw (A/D)
	mu.rotate(gmu::MODEL, pitchDeg, 1.0f, 0.0f, 0.0f);					   // slight nose tilt (UP/DOWN)
	mu.translate(gmu::MODEL, -bodyX * 0.5f, -bodyY * 0.5f, -bodyZ * 0.5f); // back

	mu.pushMatrix(gmu::MODEL);
	mu.scale(gmu::MODEL, bodyX, bodyY, bodyZ);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	float* modelMatrix = mu.get(gmu::MODEL);
	AABB aabbBox = updateGlobalAABB(drone.aabb, modelMatrix); //WORLD SPACEE
	drone.worldAABB = aabbBox;

	data.texMode = 1; // material shading
	data.mesh = &allMeshes.cube;
	data.texMode = 4;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	renderer.renderMesh(data);
	mu.popMatrix(gmu::MODEL);

	// 2) MOTORS (4 short white cylinders) — attached to the same parent frame
	const float motorR = 0.22f; // radius
	const float motorH = 0.15f; // height

	// TOP of the cube is at Y = bodyY because cube vertices are 0..1
	const float motorY = bodyY + motorH * 0.5f; // center sits just above the top face

	// If you want the motors TANGENT to the two edges (fully on top surface):
	auto placeMotor = [&](float x, float z)
	{
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, x, motorY, z);
		mu.scale(gmu::MODEL, motorR, motorH, motorR); // cylinder axis = Y
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		data.mesh = &allMeshes.cube;
		data.texMode = 1;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	};

	placeMotor(0.f, 0.f);
	placeMotor(bodyX, 0.f);
	placeMotor(0.f, bodyZ);
	placeMotor(bodyX, bodyZ);

	mu.popMatrix(gmu::MODEL);

	// horizontal street/road
	mu.pushMatrix(gmu::MODEL);
	{
		float streetLenX = cols * (buildingW + gap);
		mu.translate(gmu::MODEL, 5.0f, 0.2f, 0.0f);
		mu.scale(gmu::MODEL, streetWidth, 1.0f, streetLenX);
		mu.rotate(gmu::MODEL, -90.0f, 1.0f, 0.0f, 0.0f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();
		data.mesh = &allMeshes.quad;
		data.texMode = 6;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.loadIdentity(gmu::MODEL);
	}
	mu.popMatrix(gmu::MODEL);

	// --- Garden (central park) ---
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

	// place some low 'tree' cones around garden
	for (Tree &tree : trees) {
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, tree.position[0], tree.position[1], tree.position[2]);
        mu.scale(gmu::MODEL, tree.scale[0], tree.scale[1], tree.scale[2]);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

       float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(tree.aabb, modelMatrix); //WORLD SPACEE
		tree.worldAABB = aabbBox;

        // --- Render mesh ---
        data.mesh = &allMeshes.cone;
        data.texMode = 5;
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
    }

	// --- Draw buildings using buildings vector ---
	for (Building &b : buildings)
	{
		float h = buildingHeights[b.row][b.col];
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, b.x, 0, b.z);

		mu.scale(gmu::MODEL, b.width, h, b.depth);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		 // Compute world-space AABB
		float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(b.aabb, modelMatrix); //WORLD SPACEE
		b.worldAABB = aabbBox;

		data.mesh = &allMeshes.cube;
		data.texMode = 3;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();

		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	pointLights.clear();

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

        // --------- Draw lamp post ----------
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, lx, 0, lz);
        mu.scale(gmu::MODEL, 0.25f, lamp.height, 0.25f);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        float* modelMatrix = mu.get(gmu::MODEL);
		AABB aabbBox = updateGlobalAABB(lamp.aabb, modelMatrix); //WORLD SPACEE
		lamp.worldAABB = aabbBox;

        data.mesh = &allMeshes.cube;
        data.texMode = 4; // metal
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
    }
	renderer.setLampLights(pointLights, lampsOn);

	// --------- Draw bulb ----------
	glDepthMask(GL_FALSE);  // don’t write depth while blending
	for (LampPost &lamp : lampPosts) {
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, lamp.position[0], lamp.height, lamp.position[2]);
        mu.scale(gmu::MODEL, 0.6f, 0.6f, 0.6f);
        mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
        mu.computeNormalMatrix3x3();

        data.mesh = &allMeshes.sphere;
        data.texMode = 2; //create new one FIX
        data.vm = mu.get(gmu::VIEW_MODEL);
        data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
        data.normal = mu.getNormalMatrix();
        renderer.renderMesh(data);
        mu.popMatrix(gmu::MODEL);
	}
	glDepthMask(GL_TRUE);

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

	case 'h': // toggle spotlight mode
		if (!spotlight_mode)
		{
			spotlight_mode = true;
			printf("Spot light disabled\n");
		}
		else
		{
			spotlight_mode = false;
			printf("Spot light disabled. Point light enabled\n");
		}
		break;

	case 'c': // toggle lamp posts
		lampsOn = !lampsOn;
		printf("Lamp posts %s\n", lampsOn ? "ON" : "OFF");
		break;

	case 'n': // toggle day/night mode
		dayMode = !dayMode;
		if (dayMode)
			printf("Day mode ON\n");
		else
			printf("Night mode ON\n");
		break;

	case 'm':
		glEnable(GL_MULTISAMPLE);
		break;
	case 'p':
		glDisable(GL_MULTISAMPLE);
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

// Track mouse motion while buttons are pressed


void processMouseMotion(int xx, int yy) {
    int deltaX = xx - startX;
    int deltaY = yy - startY;

    if (tracking == 1) { // left mouse button: rotate camera
        followYawOffsetDeg   += deltaX * 0.3f;  // scale to taste
        followPitchOffsetDeg += deltaY * 0.3f;

        if (followPitchOffsetDeg > 85.0f) followPitchOffsetDeg = 85.0f;
        if (followPitchOffsetDeg < -85.0f) followPitchOffsetDeg = -85.0f;

        startX = xx; // update last mouse pos
        startY = yy;
    }
    else if (tracking == 2) { // right mouse button: zoom
        followDistance += deltaY * 0.1f;
        if (followDistance < 1.0f) followDistance = 1.0f;
        startX = xx;
        startY = yy;
    }
}

void mouseWheel(int wheel, int direction, int x, int y)
{

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	/* camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f); */

	//  uncomment this if not using an idle or refresh func
	//	glutPostRedisplay();
}

void processSpecialDown(int key, int x, int y)
{
	spKeys[key] = true;
}
void processSpecialUp(int key, int x, int y)
{
	spKeys[key] = false;
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

	// Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = {0.2f, 0.15f, 0.1f, 1.0f};
	float diff[] = {0.8f, 0.6f, 0.4f, 1.0f};
	float spec[] = {0.8f, 0.8f, 0.8f, 1.0f};

	float amb1[] = {0.3f, 0.0f, 0.0f, 1.0f};
	float diff1[] = {0.8f, 0.1f, 0.1f, 1.0f};
	float spec1[] = {0.3f, 0.3f, 0.3f, 1.0f};

	float ambBulb[] = {0.2f, 0.2f, 0.0f, 0.3f};	 // alpha = 0.3
	float diffBulb[] = {0.8f, 0.8f, 0.2f, 0.3f};
	float specBulb[] = {0.5f, 0.5f, 0.3f, 0.3f};
	float emisBulb[] = {1.0f, 1.0f, 0.2f, 0.3f};


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
	allMeshes.sphere = createSphere(1.0f, 16);
	memcpy(allMeshes.sphere.mat.ambient, ambBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.diffuse, diffBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.specular, specBulb, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.emissive, emisBulb, 4 * sizeof(float));
	allMeshes.sphere.mat.shininess = shininess;
	allMeshes.sphere.mat.texCount = texcount;

	// Cone
	allMeshes.cone = createCone(2.0f, 1.0f, 20);
	memcpy(allMeshes.cone.mat.ambient, amb, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.specular, spec, 4 * sizeof(float));
	memcpy(allMeshes.cone.mat.emissive, emissive, 4 * sizeof(float));
	allMeshes.cone.mat.shininess = shininess;
	allMeshes.cone.mat.texCount = texcount;

	// ----- INITIALIZE FLYING OBJECTS -----
	for (int i = 0; i < 10; i++)
	{
		FlyingObject obj;

		// random spawn position in XZ plane
		obj.position[0] = (rand() % 200 - 100);
		obj.position[1] = 10.0f + rand() % 30; // altitude between 10–40
		obj.position[2] = (rand() % 200 - 100);

		// random direction in XZ plane
		float angle = (rand() % 360) * 3.14159f / 180.0f;
		obj.direction[0] = cos(angle);
		obj.direction[1] = 0.0f;
		obj.direction[2] = sin(angle);

		obj.speed = 0.05f + (rand() % 10) * 0.01f; // 0.05 – 0.15
		obj.rotationAngle = 0.0f;
		obj.rotationSpeed = 1.0f + (rand() % 5); // 1 – 5 degrees per frame
		obj.meshID = 2 + (i % 2);				 // 2 = sphere, 3 = cone
		obj.active = true;
		obj.aabb = allMeshes.cube.aabb;
		flyingObjects.push_back(obj);
	}

	// Clear and fill building vector
	buildings.clear();
	for (int r = 0; r < rows; ++r)
	{
		for (int c = 0; c < cols; ++c)
		{
			// compute standard position center for this grid cell
			float x = offsetX + c * (buildingW + gap);
			float z = offsetZ + r * (buildingD + gap);

			bool isStreet = false;
			// create vertical street down the middle
			if (c == cols / 2)
				isStreet = true;
			// create horizontal street across middle
			if (r == rows / 2)
				isStreet = true;

			// carve out the central garden area (a rectangle around center)
			int gardenRowStart = rows / 2 - gardenSizeRows / 2;
			int gardenRowEnd = gardenRowStart + gardenSizeRows - 1;
			int gardenColStart = cols / 2 - gardenSizeCols / 2;
			int gardenColEnd = gardenColStart + gardenSizeCols - 1;
			bool isGardenCell = (r >= gardenRowStart && r <= gardenRowEnd && c >= gardenColStart && c <= gardenColEnd);

			if (!isStreet && !isGardenCell)
			{
				Building b;
				b.x = x;
				b.z = z;
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
				// leave empty: street or garden
				buildingHeights[r][c] = 0.0f;
			}
		}
	}

	float lampRadius = std::max(gardenW, gardenD) / 2.0f; 
	int lampCount = LAMP_POST_NUMBER;
    for (int i = 0; i < lampCount; ++i) {
        float ang = (2.0f * 3.14159f) * i / lampCount;
        float lx = gardenCenterX + lampRadius * cos(ang);
        float lz = gardenCenterZ + lampRadius * sin(ang);
        float ly = lampHeight;

        LampPost lamp;
        lamp.position[0] = lx;
        lamp.position[1] = 0.0f;   // base at ground
        lamp.position[2] = lz;
        lamp.height = ly;
        lamp.aabb = allMeshes.cube.aabb;

        lampPosts.push_back(lamp);
    }

	for (int t = 0; t < 6; ++t) {
        float ang = (2.0f * 3.14159f) * t / 6.0f;
        float rx = gardenCenterX + (gardenW / 2.2f - 4.0f) * cos(ang);
        float rz = gardenCenterZ + (gardenD / 2.2f - 4.0f) * sin(ang);

        Tree tree;
        tree.position[0] = rx;
        tree.position[1] = 0.0f;  // base at ground
        tree.position[2] = rz;

        tree.scale[0] = 1.5f;
        tree.scale[1] = 4.0f;
        tree.scale[2] = 1.5f;
        tree.aabb = allMeshes.cone.aabb; // local bounding box

        trees.push_back(tree);
    }

	drone.position[0] = 20.0f;
	drone.position[1] = 20.0f;
	drone.position[2] = -20.0f;

	drone.direction[0] = 0.0f;
	drone.direction[1] = 0.0f;
	drone.direction[2] = -1.0f;
	drone.aabb = allMeshes.cube.aabb; //cube

	cams[0].camPos[1] = 200.0;
	cams[0].camPos[0] = 0.0;
	cams[0].camPos[2] = 0.33;
	// top ortho
	cams[1].camPos[1] = 200.0;
	cams[1].camPos[2] = 0.33;
	cams[1].type = 1;
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
	glutIdleFunc(renderSim); // Use it for maximum performance
	// glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

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

	if (!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") ||
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
		return (1);

	//  GLUT main loop
	glutMainLoop();

	return (0);
}