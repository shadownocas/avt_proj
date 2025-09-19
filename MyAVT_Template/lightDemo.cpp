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
#include <cmath>   // sinf, cosf, atan2f
#include <algorithm> // if you use std::clamp elsewhere


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
// Follow-camera mouse offsets (degrees)
float followYawOffsetDeg = 0.f;
float followPitchOffsetDeg = 15.f;   // slight downward tilt by default
float minPitchDeg = -10.0f, maxPitchDeg = 85.0f;
// mouse sensitivity & state
float MOUSE_SENS_YAW = 0.25f;    // deg per pixel
float MOUSE_SENS_PITCH = 0.25f;  // deg per pixel
float startYawDeg = 0.f, startPitchDeg = 0.f;
float startFollowDistance = 20.f;
// Zoom sensitivity (world units per pixel of RMB drag)
float ZOOM_SENS = 0.05f;

float sYawDeg = 0.0f, sPitchDeg = 15.0f, sDist = 15.0f; // init to your defaults
int prevX = 0, prevY = 0;


// --- Drone flight state (Task 4)
float yawDeg   = 0.0f;            // heading angle (deg)
float pitchDeg = 0.0f;            // nose up/down (+ = back)
float rollDeg  = 0.0f;            // bank left/right (+ = left)

float velX = 0.0f, velY = 0.0f, velZ = 0.0f;  // world velocities

// Controls & physics
const float maxTiltDeg      = 25.0f;     // clamp for pitch/roll
const float tiltStepDeg     = 1.5f;      // degrees added per frame while arrow held
const float tiltReturnDeg   = 1.0f;      // auto-level speed per frame when released
const float accelGain       = 0.02f;     // horizontal accel per frame from tilt
const float horizDrag       = 0.02f;     // decay per frame (0..1)
const float vertAccel       = 0.01f;     // W/S vertical accel per frame
const float vertDrag        = 0.01f;     // vertical decay per frame
const float maxHorizSpeed   = 0.6f;      // clamp XY speed
const float maxVertSpeed    = 0.5f;      // clamp Y speed

// Special (arrow) key states
bool spKeys[256] = {false};       // GLUT special keys (UP/DOWN/LEFT/RIGHT)

bool keyStates[256] = {false};

struct Drone {
    float position[3] = {20.0f, 20.0f, -20.0f}; // xyz
    float direction[3] = {0.0f, 0.0f, -1.0f}; // pointing along -Z initially
	float speed;
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

std::vector<PointLight> pointLights;

std::vector<FlyingObject> flyingObjects;

Drone drone;
float droneSpeed = 0.2f;       // units per frame
float followDistance = 15.0f;  // camera distance behind drone
float followHeight = 5.0f;     // camera height above drone
float droneRotSpeed = 1.5f;    // degrees per frame

struct Camera {
  float camPos[4] = {0.0f, 0.0f, 0.0f, 0.0f}; //camera 4 for orbit view of drone
  float camTarget[3] = {0.0f, 0.0f, 0.0f};
  int type=0; //0:perspective, 1:orthographic
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
long myTime,timebase = 0,frame = 0;
char s[32];

bool dayMode = true;                 // Day/night toggle
float dirLightDir[3] = { -0.5f, -1.0f, -0.3f };  // Direction of sunlight
float dirLightColor[3] = { 1.0f, 1.0f, 0.9f };   // Day color
//float nightLightColor[3] = { 0.1f, 0.1f, 0.2f }; // Night ambient color
float nightLightColor[3] = { 0.0f, 0.0f, 0.0f };


float lightPos[4] = {4.0f, 20.0f, 2.0f, 1.0f};
//float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

//Spotlight
bool spotlight_mode = false;
float coneDir[4] = { 0.0f, -0.0f, -1.0f, 0.0f };

SpotLight droneHeadlights[2]; // two headlights

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
	int row, col;    // indices no grid
};

std::vector<Building> buildings;
std::vector<std::vector<float>> buildingHeights(rows, std::vector<float>(cols));

float buildingW = 10.0f;
float buildingD = 10.0f;
float streetWidth = 30.0f;    // largura da rua
float sidewalk = 2.0f;       // passeios

// central garden define: ocupa N x M células no centro
int gardenSizeRows = 3; // 2x2 cells
int gardenSizeCols = 3;

float gardenHalfW = (gardenSizeCols * (buildingW + gap) - gap) / 2.0f;
float gardenHalfD = (gardenSizeRows * (buildingD + gap) - gap) / 2.0f;

float gardenCenterX = 6.0f; // because grid centered with offsetX/offsetZ
float gardenCenterZ = 5.0f;
float gardenW = gardenHalfW * 2.0f;
float gardenD = gardenHalfD * 2.0f;

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

void refresh(int value) // faz RENDER
{
	glutPostRedisplay();
	glutTimerFunc(1000 / 60, refresh, 0);
}

void animate(){ //UPDATE das posicoes e no render desenha!
	/* vel = theta * ...;
	pos += vel * deltaT */
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

	WinX = w;
    WinY = h;
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
void updateFlight() {
    const float D2R = 3.1415926f / 180.0f;
	if (keyStates['w']){
		drone.position[0] += drone.direction[0] * droneSpeed;
		drone.position[2] += drone.direction[2] * droneSpeed;
	}
	if (keyStates['s']){
		drone.position[0] -= drone.direction[0] * droneSpeed;
		drone.position[2] -= drone.direction[2] * droneSpeed;
	}
    // --- Yaw (A/D): adjust heading, then rebuild forward vector
    if (keyStates['a']) yawDeg -= droneRotSpeed;   // deg/frame (uses your existing var)
    if (keyStates['d']) yawDeg += droneRotSpeed;

    float yawRad = yawDeg * D2R;
    drone.direction[0] = std::sin(yawRad);
    drone.direction[1] = 0.0f;
    drone.direction[2] = -std::cos(yawRad);

    // --- Pitch & Roll (Arrow keys): change tilt, with auto-level when released
    if (spKeys[GLUT_KEY_LEFT])  rollDeg += tiltStepDeg;
    if (spKeys[GLUT_KEY_RIGHT]) rollDeg -= tiltStepDeg;
    if (!spKeys[GLUT_KEY_LEFT] && !spKeys[GLUT_KEY_RIGHT]) {
        if (rollDeg > 0)  rollDeg = std::max(0.0f, rollDeg - tiltReturnDeg);
        if (rollDeg < 0)  rollDeg = std::min(0.0f, rollDeg + tiltReturnDeg);
    }

    if (spKeys[GLUT_KEY_UP])    pitchDeg -= tiltStepDeg;  // nose down -> forward accel
    if (spKeys[GLUT_KEY_DOWN])  pitchDeg += tiltStepDeg;  // nose up   -> backward accel
    if (!spKeys[GLUT_KEY_UP] && !spKeys[GLUT_KEY_DOWN]) {
        if (pitchDeg > 0) pitchDeg = std::max(0.0f, pitchDeg - tiltReturnDeg);
        if (pitchDeg < 0) pitchDeg = std::min(0.0f, pitchDeg + tiltReturnDeg);
    }

    // clamp tilts
    pitchDeg = std::max(-maxTiltDeg, std::min(maxTiltDeg, pitchDeg));
    rollDeg  = std::max(-maxTiltDeg, std::min(maxTiltDeg,  rollDeg));

    // --- Acceleration from tilt (local -> world)
    // local axes: +X = right wing, +Z = forward. Nose down (negative pitch) pushes forward.
    float ax_local = std::sin(rollDeg * D2R)  * accelGain;   // left/right
    float az_local = -std::sin(pitchDeg * D2R) * accelGain;  // forward/back

    float cosY = std::cos(yawRad), sinY = std::sin(yawRad);
    float ax = ax_local * cosY + az_local * sinY;
    float az = -ax_local * sinY + az_local * cosY;

    velX += ax;
    velZ += az;

    // --- Throttle (W/S): vertical speed
    if (keyStates['w']) velY += vertAccel;
    if (keyStates['s']) velY -= vertAccel;

    // --- Drag (gradual slow down when not leaning)
    velX *= (1.0f - horizDrag);
    velZ *= (1.0f - horizDrag);
    velY *= (1.0f - vertDrag);

    // --- Clamp speeds
    float h = std::sqrt(velX*velX + velZ*velZ);
    if (h > maxHorizSpeed) { float k = maxHorizSpeed / h; velX *= k; velZ *= k; }
    if (velY >  maxVertSpeed) velY =  maxVertSpeed;
    if (velY < -maxVertSpeed) velY = -maxVertSpeed;

    // --- Integrate position
    drone.position[0] += velX;
    drone.position[1] += velY;
    drone.position[2] += velZ;
}

void updateCamera(){
	float ratio = (float)WinX / (float)WinY;

	mu.loadIdentity(gmu::PROJECTION);

	if (activeCam == 0 || activeCam == 2) {
		mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	} else if (activeCam == 1 ) {
		float orthoSize = 60.0f;
		mu.ortho(-orthoSize * ratio, orthoSize * ratio, -orthoSize, orthoSize, -500.0f, 500.0f);
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
	renderer.setTexUnit(5, 5);
	renderer.setTexUnit(6, 6);

    // Update drone movement
    updateFlight();
    updateCamera();

    // --- Set camera/view ---
    mu.loadIdentity(gmu::VIEW);
    mu.loadIdentity(gmu::MODEL);


	// ----- UPDATE FLYING OBJECTS -----
	for (auto &obj : flyingObjects) {
		// Move forward
		obj.position[0] += obj.direction[0] * obj.speed;
		obj.position[1] += obj.direction[1] * obj.speed;
		obj.position[2] += obj.direction[2] * obj.speed;

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

	// --- Follow camera (activeCam == 2): orbit around drone forward with mouse offsets
if (activeCam == 2) {
        const float k = 0.20f; // 0..1, higher = snappier
    sYawDeg   += (followYawOffsetDeg   - sYawDeg)   * k;
    sPitchDeg += (followPitchOffsetDeg - sPitchDeg) * k;
    sDist     += (followDistance       - sDist)     * k;

    // --- Build follow camera from smoothed values ---
    const float DEG2RAD = 3.1415926f / 180.0f;

    // Pivot at fixed height over the drone (prevents “zoomy” feeling)
    float pivotX = drone.position[0];
    float pivotY = drone.position[1] + followHeight;
    float pivotZ = drone.position[2];

    // Camera position: orbit on XZ using YAW only (fixed radius sDist)
    float baseYaw = atan2f(drone.direction[0], -drone.direction[2]); // radians
    float yawCam  = baseYaw + (sYawDeg + 180.0f) * DEG2RAD;  // put cam behind at yaw=0
	float ox = sinf(yawCam), oz = cosf(yawCam);

    cams[2].camPos[0] = pivotX - ox * sDist;
    cams[2].camPos[1] = pivotY;                  // fixed height
    cams[2].camPos[2] = pivotZ - oz * sDist;

    // Camera target: use PITCH to tilt the VIEW (not the position)
    float pitchCam = sPitchDeg * DEG2RAD;

    // forward (XZ) normalized
    float fx = drone.direction[0], fz = drone.direction[2];
    float fl = std::sqrt(fx*fx + fz*fz); if (fl < 1e-6f) { fx = 0.f; fz = -1.f; fl = 1.f; }
    fx /= fl; fz /= fl;

    float lx = fx * std::cos(pitchCam);
    float ly = std::sin(pitchCam);
    float lz = fz * std::cos(pitchCam);

    float lookAhead = 40.0f;
    cams[2].camTarget[0] = pivotX + lx * lookAhead;
    cams[2].camTarget[1] = pivotY + ly * lookAhead;
    cams[2].camTarget[2] = pivotZ + lz * lookAhead;
}

//camera for orbit view of the drone

	// set the camera using a function similar to gluLookAt
	mu.lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
	cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2], 0,1,0);

    // Directional light
    float dirLightWorld[4] = { 0.5f, -0.7f, 0.3f, 0.0f };
    float dirLightEye[4];
    mu.multMatrixPoint(gmu::VIEW, dirLightWorld, dirLightEye);
    float dirLightEye3[3] = { dirLightEye[0], dirLightEye[1], dirLightEye[2] };
	renderer.setDirectionalLight(dirLightEye3, dayMode ? dirLightColor : nightLightColor);

	dataMesh data;

	pointLights.clear();

	int lampCount = 6;
	float lampRadius = std::max(gardenW, gardenD) / 2.0f; // postes ligeiramente fora do jardim

	for (int i = 0; i < lampCount; ++i) {
		float ang = (2.0f * 3.14159f) * i / lampCount;
		float lx = gardenCenterX + lampRadius * cos(ang);
		float lz = gardenCenterZ + lampRadius * sin(ang);
		float ly = lampHeight;

		float worldPos[4] = { lx, ly, lz, 1.0f };
		float eyePos[4];
		mu.multMatrixPoint(gmu::VIEW, worldPos, eyePos);

		PointLight lamp;
		lamp.LocalPos[0] = eyePos[0];
		lamp.LocalPos[1] = eyePos[1];
		lamp.LocalPos[2] = eyePos[2];
		lamp.Color[0] = 3.0f; 
		lamp.Color[1] = 2.7f;
		lamp.Color[2] = 2.1f;
		lamp.atten.constant = 1.0f;
		lamp.atten.linear   = 0.02f;
		lamp.atten.exp      = 0.02f;
		pointLights.push_back(lamp);

		//LampPost
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, lx, 0, lz);
		mu.scale(gmu::MODEL, 0.25f, ly, 0.25f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();
		data.mesh = &allMeshes.cube; // cube
		data.texMode = 4; // metal
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);

		// and small emissive sphere at top as bulb
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, lx, ly, lz);
		mu.scale(gmu::MODEL, 0.6f, 0.6f, 0.6f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();
		data.mesh = &allMeshes.sphere; // sphere mesh
		data.texMode = 2;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();

		// set emissive on mesh material somehow (if renderer supports per-mesh emissive)
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}
	renderer.setLampLights(pointLights, lampsOn);

	// --- Headlight offsets in LOCAL space of the cube ---
	float localHeadlightOffsets[2][4] = {
		{ -0.5f,  0.0f,  1.0f, 1.0f },  // left headlight
		{  0.5f,  0.0f,  1.0f, 1.0f }   // right headlight
	};

	// Compute drone's rotation matrix
	float angle_Y = atan2(drone.direction[0], -drone.direction[2]);
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, drone.position[0], drone.position[1], drone.position[2]);
	mu.rotate(gmu::MODEL, angle_Y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);

	// For each headlight
	for (int i = 0; i < 2; i++) {
		float worldPos[4];
		mu.multMatrixPoint(gmu::MODEL, localHeadlightOffsets[i], worldPos);

		// Convert to eye space
		float eyePos[4];
		mu.multMatrixPoint(gmu::VIEW, worldPos, eyePos);

		droneHeadlights[i].Position[0] = eyePos[0];
		droneHeadlights[i].Position[1] = eyePos[1];
		droneHeadlights[i].Position[2] = eyePos[2];

		// Forward direction of drone (local Z axis)
		float localDir[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
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
		droneHeadlights[i].atten.linear   = 0.1f;
		droneHeadlights[i].atten.exp      = 0.01f;
		droneHeadlights[i].Cutoff = cosf(20.0f * 3.14159f / 180.0f);
	}
	mu.popMatrix(gmu::MODEL);

	renderer.setDroneSpotLights(droneHeadlights, 2, spotlight_mode);

		// ----- RENDER FLYING OBJECTS -----
	for (auto &obj : flyingObjects) {
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, obj.position[0], obj.position[1], obj.position[2]);
		mu.rotate(gmu::MODEL, obj.rotationAngle, 0.0f, 1.0f, 0.0f);

		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

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
    mu.rotate(gmu::MODEL,-90.0f, 1.0f, 0.0f, 0.0f);
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

	// Body size (keep these up here so we can use them for the pivot)
	const float bodyX = 1.6f, bodyY = 0.25f, bodyZ = 1.6f;

	// yaw (from direction)
	float angleY = atan2(drone.direction[0], -drone.direction[2]) * 180.0f / 3.14159f;

	// >>> move pivot to the cube center, rotate, then move back <<<
	mu.translate(gmu::MODEL,  bodyX * 0.5f, bodyY * 0.5f, bodyZ * 0.5f); // to center
	mu.rotate(   gmu::MODEL,  angleY,       0.0f, 1.0f, 0.0f);            // yaw
	mu.translate(gmu::MODEL, -bodyX * 0.5f,-bodyY * 0.5f,-bodyZ * 0.5f);  // back

	// 1) BODY (scaled green cube)

    mu.pushMatrix(gmu::MODEL);
    mu.scale(gmu::MODEL, bodyX, bodyY, bodyZ);
    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    mu.computeNormalMatrix3x3();
    
    data.texMode = 1;                // material shading
    data.mesh = &allMeshes.cube;
    data.texMode = 4;
    data.vm = mu.get(gmu::VIEW_MODEL);
    data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
    data.normal = mu.getNormalMatrix();
    renderer.renderMesh(data);
    mu.popMatrix(gmu::MODEL);


	// --- Draw streets (as long quads with asphalt texture) ---
	/* mu.pushMatrix(gmu::MODEL);
	{   // vertical street (central column)
		float streetLenZ = rows * (buildingD + gap);
		mu.translate(gmu::MODEL, 0.0f, 0.01f, 0.0f); // slightly above floor
		mu.rotate(gmu::MODEL, 90.0f, 0.0f, 1.0f, 0.0f);
		mu.rotate(gmu::MODEL, -90.0f, 1.0f, 0.0f, 0.0f);
		mu.scale(gmu::MODEL, streetWidth, 1.0f, streetLenZ);
		//mu.rotate(gmu::MODEL, -90.0f, 0.0f, 0.0f, 1.0f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();
		data.mesh = &allMeshes.quad;        // quad mesh
		data.texMode = 6;      // texture unit 0 -> asphalt / stone.tga
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.loadIdentity(gmu::MODEL);
	}
	mu.popMatrix(gmu::MODEL); */

// 2) MOTORS (4 short white cylinders) — attached to the same parent frame

    const float motorR = 0.22f;   // radius
    const float motorH = 0.15f;   // height

    // TOP of the cube is at Y = bodyY because cube vertices are 0..1
    const float motorY = bodyY + motorH * 0.5f;   // center sits just above the top face

    // If you want the motors TANGENT to the two edges (fully on top surface):
    auto placeMotor = [&](float x, float z) {
        mu.pushMatrix(gmu::MODEL);
        mu.translate(gmu::MODEL, x, motorY, z);
        mu.scale(gmu::MODEL, motorR, motorH, motorR);  // cylinder axis = Y
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
		mu.rotate(gmu::MODEL,-90.0f, 1.0f, 0.0f, 0.0f);
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

	// --- Draw garden (central park) ---
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, gardenCenterX, 0.3f, gardenCenterZ);
	mu.scale(gmu::MODEL, gardenW, 1.0f, gardenD);
	mu.rotate(gmu::MODEL,-90.0f, 1.0f, 0.0f, 0.0f);
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
	for (int t = 0; t < 6; ++t) {
		float ang = (2.0f * 3.14159f) * t / 6.0f;
		float rx = gardenCenterX + (gardenW/2.2f - 4.0f) * cos(ang);
		float rz = gardenCenterZ + (gardenD/2.2f - 4.0f) * sin(ang);
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, rx, 0, rz);
		mu.scale(gmu::MODEL, 1.5f, 4.0f, 1.5f);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		data.mesh = &allMeshes.cone;
		data.texMode = 5;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();
		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

	// --- Draw buildings using buildings vector ---
	for (const Building &b : buildings) {
		float h = buildingHeights[b.row][b.col]; 
		mu.pushMatrix(gmu::MODEL);
		mu.translate(gmu::MODEL, b.x, 0, b.z);

		mu.scale(gmu::MODEL, b.width, h, b.depth);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		mu.computeNormalMatrix3x3();

		data.mesh = &allMeshes.cube;
		data.texMode = 3;
		data.vm = mu.get(gmu::VIEW_MODEL);
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		data.normal = mu.getNormalMatrix();

		renderer.renderMesh(data);
		mu.popMatrix(gmu::MODEL);
	}

/** 
   // Camera position: fixed behind and above drone
	cams[2].camPos[0] = drone.position[0] - drone.direction[0] * followDistance;
	cams[2].camPos[1] = drone.position[1] + followHeight;
	cams[2].camPos[2] = drone.position[2] - drone.direction[2] * followDistance;

	// Camera target: in front of the drone (not at the drone itself)
	float lookAhead = 50.0f; // how far forward camera looks
	cams[2].camTarget[0] = drone.position[0] + drone.direction[0] * lookAhead;
	cams[2].camTarget[1] = drone.position[1];
	cams[2].camTarget[2] = drone.position[2] + drone.direction[2] * lookAhead;

**/
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

		case 'h':   //toggle spotlight mode
			if (!spotlight_mode) {
				spotlight_mode = true;
				printf("Spot light disabled\n");
			}
			else {
				spotlight_mode = false;
				printf("Spot light disabled. Point light enabled\n");
			}
			break;

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
    if (state == GLUT_DOWN)  {
        startX = prevX = xx;
        startY = prevY = yy;

        if (button == GLUT_LEFT_BUTTON) {
            tracking = 1; // orbit
            // no need for startYawDeg/startPitchDeg with incremental mode
        }
        else if (button == GLUT_RIGHT_BUTTON) {
            tracking = 2; // zoom
            startFollowDistance = followDistance;
        }
    } else if (state == GLUT_UP) {
        tracking = 0;
    }
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{
    int dx = xx - prevX;
    int dy = yy - prevY;
    prevX = xx; prevY = yy;

    if (activeCam == 2) {
        if (tracking == 1) {             // ORBIT (LMB)
            followYawOffsetDeg   -= dx * MOUSE_SENS_YAW;   // flip sign if feels inverted
            followPitchOffsetDeg += dy * MOUSE_SENS_PITCH;

            // clamp pitch only
            if (followPitchOffsetDeg > maxPitchDeg)       followPitchOffsetDeg = maxPitchDeg;
            else if (followPitchOffsetDeg < minPitchDeg)  followPitchOffsetDeg = minPitchDeg;

            // TEMP: remove yaw wrap entirely while testing smooth 360
            // (comment out your two lines below)
            // if (followYawOffsetDeg > 180.f)  followYawOffsetDeg -= 360.f;
            // if (followYawOffsetDeg < -180.f) followYawOffsetDeg += 360.f;
        }
        else if (tracking == 2) {         // ZOOM (RMB)
            followDistance = startFollowDistance + (yy - startY) * ZOOM_SENS;
            if (followDistance < 5.0f)  followDistance = 5.0f;
            if (followDistance > 60.0f) followDistance = 60.0f;
        }
        return;
    }
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


void processSpecialDown(int key, int x, int y) {
    spKeys[key] = true;
}
void processSpecialUp(int key, int x, int y) {
    spKeys[key] = false;
}


//
// Scene building with basic geometry
//

void buildScene()
{
	//Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/pavement.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");
	renderer.TexObjArray.texture2D_Loader("assets/Bricks097.tga");  
	renderer.TexObjArray.texture2D_Loader("assets/metal.tga");  
	renderer.TexObjArray.texture2D_Loader("assets/grass.tga");  
	renderer.TexObjArray.texture2D_Loader("assets/road.tga"); 

	//Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	float ambBulb[]  = {0.2f, 0.2f, 0.0f, 1.0f};   // slight yellow base
	float diffBulb[] = {0.8f, 0.8f, 0.2f, 1.0f};   // bright yellow diffuse
	float specBulb[] = {0.5f, 0.5f, 0.3f, 1.0f};   // soft highlight
	float emisBulb[] = {1.0f, 1.0f, 0.2f, 1.0f};   // strong emissive glow (neon!)


	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
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

// --- GREEN cube dedicated to the drone body ---
MyMesh droneCube = createCube();
float ambG[]  = {0.05f, 0.15f, 0.05f, 1.0f};
float diffG[] = {0.15f, 0.80f, 0.15f, 1.0f};  // green
float specG[] = {0.20f, 0.90f, 0.20f, 1.0f};
memcpy(droneCube.mat.ambient,  ambG,  4*sizeof(float));
memcpy(droneCube.mat.diffuse,  diffG, 4*sizeof(float));
memcpy(droneCube.mat.specular, specG, 4*sizeof(float));
memcpy(droneCube.mat.emissive, emissive, 4*sizeof(float));
droneCube.mat.shininess = 80.0f;
droneCube.mat.texCount  = 0;

renderer.myMeshes.push_back(droneCube);


// --- Motor mesh (WHITE cylinder) ---
MyMesh cyl = createCylinder(1.0f, 1.0f, 24);

float ambW[]  = {0.20f, 0.20f, 0.20f, 1.0f};  // small ambient
float diffW[] = {0.95f, 0.95f, 0.95f, 1.0f};  // white diffuse
float specW[] = {1.00f, 1.00f, 1.00f, 1.0f};  // bright specular
memcpy(cyl.mat.ambient,  ambW,  4*sizeof(float));
memcpy(cyl.mat.diffuse,  diffW, 4*sizeof(float));
memcpy(cyl.mat.specular, specW, 4*sizeof(float));
cyl.mat.shininess = 120.0f;                   // shiny
cyl.mat.texCount  = 0;

	renderer.myMeshes.push_back(cyl);

	drone.position[0] = 20.0f;
	drone.position[1] = 20.0f;
	drone.position[2] = -20.0f;
	// Sphere
	allMeshes.sphere = createSphere(1.0f, 16);
	memcpy(allMeshes.sphere.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.specular, spec1, 4 * sizeof(float));
	memcpy(allMeshes.sphere.mat.emissive, emissive, 4 * sizeof(float));
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

	// Clear and fill building vector
	buildings.clear();
	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			// compute standard position center for this grid cell
			float x = offsetX + c * (buildingW + gap);
			float z = offsetZ + r * (buildingD + gap);

			bool isStreet = false;
			// create vertical street down the middle
			if (c == cols/2) isStreet = true;
			// create horizontal street across middle
			if (r == rows/2) isStreet = true;

			// carve out the central garden area (a rectangle around center)
			int gardenRowStart = rows/2 - gardenSizeRows/2;
			int gardenRowEnd   = gardenRowStart + gardenSizeRows - 1;
			int gardenColStart = cols/2 - gardenSizeCols/2;
			int gardenColEnd   = gardenColStart + gardenSizeCols - 1;
			bool isGardenCell = (r >= gardenRowStart && r <= gardenRowEnd && c >= gardenColStart && c <= gardenColEnd);

			if (!isStreet && !isGardenCell) {
				Building b;
				b.x = x;
				b.z = z;
				b.width = buildingW;
				b.depth = buildingD;
				b.row = r;
    			b.col = c;
				buildings.push_back(b);

				buildingHeights[r][c] = std::max((rand() % 20) + 8.0f, 10.0f);
			} else {
				// leave empty: street or garden
				buildingHeights[r][c] = 0.0f;
			}
		}
	}

	drone.position[0] = 20.0f;
	drone.position[1] = 20.0f;
	drone.position[2] = -20.0f;

	drone.direction[0] = 0.0f;
	drone.direction[1] = 0.0f;
	drone.direction[2] = -1.0f;

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

	buildScene();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}



