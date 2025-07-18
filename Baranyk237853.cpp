#include "imageLoad.h"
#include <cmath>
#include <GL/glut.h>
#include <string>
#include "OBJ_Loader.h"

// konstanty a makra pridejte zde
// PI/180
#define PI        3.141592653589793f
#define PIover180 0.017453292519943f
#define MPI4 0.78539816339744830962f

// globalni promenne pro transformace
float xnew = 0, ynew = 0;
float xold = 0, yold = 0;
float xx = 0, yy = 0;
bool tocime = false;
float uhel = 0;

float tranz = -100.0f;
float tranx = 0.0f;
float trany = 0.0f;

float radius = 150;

float stepHeight = 2.0f;
float stepDuration = 0.3f;
bool isStepping = false;
float stepTime = 0.0f;
float maxStepHeight = 2.0f;

bool enableTreeBillboarding = true;
bool enableStepping = false;
bool enableRotation = false;
bool enablePlaneTexture = true;

int stamina = 200;
const int maxStamina = 200;

int buttonX, buttonY, buttonWidth = 120, buttonHeight = 40;
bool buttonClicked = false;

float tranx_prev = 0.0f;
float trany_prev = 0.0f;
float tranz_prev = -100.0f;

float xOffset = 0.0f;
float rOffset = 0.0f;

float treeSpinOffset = 0.0f;

std::string lastCommand = "No input.... yet";

// nastaveni projekce
float fov = 65.0;
float nearPlane = 0.1;
float farPlane = 500.0;

// zde vlastnosti svetel materialu, menu, popripade jinych vlastnosti
// ...

unsigned int planeTextureID;
unsigned int headTextureID;

objl::Loader vodkaLoader;
GLuint vodkaTexture;


void generateHeadTexture(int size = 64)
{
	GLubyte* data = new GLubyte[size * size * 3];

	for (int y = 0; y < size; ++y)
	{
		for (int x = 0; x < size; ++x)
		{
			int checker = ((x / 8) % 2) ^ ((y / 8) % 2);
			int index = (y * size + x) * 3;

			if (checker)
			{
				data[index] = 255;    
				data[index + 1] = 0; 
				data[index + 2] = 190;  
			}
			else
			{
				data[index] = 255;
				data[index + 1] = 255;
				data[index + 2] = 255;
			}
		}
	}

	glGenTextures(1, &headTextureID);
	glBindTexture(GL_TEXTURE_2D, headTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	delete[] data;
}

// refresh funkce, smooth pocit 
void OnIdle() {
	glutPostRedisplay(); 
}

// obsluha svetla
void LightingMenuHandler(int option) {
	switch (option) {
	case 1:
		glEnable(GL_LIGHTING);
		lastCommand = "Lighting ON";
		break;
	case 2:
		glDisable(GL_LIGHTING);
		lastCommand = "Lighting OFF";
		break;
	}
	glutPostRedisplay();
}

// funkce pro obsluhu menu
void MenuHandler(int option) {
	switch (option) {
	case 0: exit(0); break;
	case 3:
		xnew = ynew = xold = yold = 0;
		tranx = 0;
		tranz = -100.0f;
		lastCommand = "Camera reset";
		break;
	case 4:
		enableStepping = !enableStepping;
		lastCommand = enableStepping ? "Headbobbing enabled" : "Headbobbing disabled";
		break;
	case 5:
		enableRotation = !enableRotation;
		lastCommand = enableRotation ? "Object rotation ON" : "Object rotation OFF";
		break;
	case 6:
		enableTreeBillboarding = !enableTreeBillboarding;
		lastCommand = enableTreeBillboarding ? "Tree billboarding ON" : "Tree billboarding OFF";
		break;
	case 7:
		enablePlaneTexture = !enablePlaneTexture;
		lastCommand = enablePlaneTexture ? "Plane texture ON" : "Plane texture OFF";
		break;
	case 8:
		stamina = maxStamina;
		lastCommand = "Stamina refilled!";
	}
}

// funkce pro vytvoøení menu
void CreateMainMenu() {
	int lightingMenu = glutCreateMenu(LightingMenuHandler);
	glutAddMenuEntry("ON", 1);
	glutAddMenuEntry("OFF", 2);

	int menu = glutCreateMenu(MenuHandler);
	glutAddMenuEntry("Exit", 0);
	glutAddSubMenu("Lighting", lightingMenu);
	glutAddMenuEntry("Camera RESET", 3);
	glutAddMenuEntry("Toggle Stepping", 4);
	glutAddMenuEntry("Toggle Object Rotation", 5);
	glutAddMenuEntry("Toggle Tree Billboarding", 6);
	glutAddMenuEntry("Toggle Plane Texture", 7);
	glutAddMenuEntry("Refill Stamina", 8);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// funkce volana pri zmene velikosti sceny
void OnReshape(int w, int h)            // event handler pro zmenu velikosti okna
{
	glViewport(0, 0, w, h);             // OpenGL: nastaveni rozmenu viewportu
	glMatrixMode(GL_PROJECTION);        // OpenGL: matice bude typu projekce
	glLoadIdentity();                   // OpenGL: matice bude identicka (jen jednicky v hlavni diagonale)
	gluPerspective(fov, (double)w / h, nearPlane, farPlane); // perspektivni projekce
}

// funkce pro inicializaci
// do teto funkce vlozit vsechny vlastnosti a inicializace, ktere se nebudou menit pri behu aplikace
void OnInit(void)
{
	// Lighting setup
	glEnable(GL_LIGHTING);               // Enable lighting
	glEnable(GL_LIGHT0);                 // Enable light #0
	glEnable(GL_COLOR_MATERIAL);         // Use glColor for material
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat diffuseLight[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	generateHeadTexture();

	if (!setTexture("sky.tga", &planeTextureID, true)) {
		std::cout << "Failed to load plane texture!" << std::endl;
	}

	if (!vodkaLoader.LoadFile("vodka.obj")) {
		std::cout << "Failed to load vodka.obj!" << std::endl;
	}

	if (!setTexture("vodka.bmp", &vodkaTexture, true)) {
		std::cout << "Failed to load vodka texture!" << std::endl;
	}

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	glFrontFace(GL_CW);
	glPolygonMode(GL_FRONT, GL_FILL);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	CreateMainMenu();
}


// doporucene vykresleni objektu 
// (1) pouzijte PUSH/POP pro GL_MODELVIEW matice, abyste si nerozhodili transformace cele sceny
// (2) vykreslujte vlastni objekty okolo pocatku (0,0,0) pro snazsi provadeni transformaci a umisteni ve scene
// (3) pro ziskaji aktulanich stavovych promennych v OpenGL muzete pouzit funkce glGetBooleanv(...)
void DrawPlane(int size)
{
	glPushMatrix();
	glTranslatef(0, -15, 0);

	GLboolean isLightingEnabled;
	glGetBooleanv(GL_LIGHTING, &isLightingEnabled);
	if (isLightingEnabled) glDisable(GL_LIGHTING);

	if (enablePlaneTexture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, planeTextureID);
		glColor3f(1.0f, 1.0f, 1.0f);

		float tiles = 10.0f;
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3i(-size, 0, size);
		glTexCoord2f(0.0f, tiles); glVertex3i(-size, 0, -size);
		glTexCoord2f(tiles, tiles); glVertex3i(size, 0, -size);
		glTexCoord2f(tiles, 0.0f); glVertex3i(size, 0, size);
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		glColor3f(0.0, 0.7, 0.0); 
		glBegin(GL_QUADS);
		glVertex3i(-size, 0, size);
		glVertex3i(-size, 0, -size);
		glVertex3i(size, 0, -size);
		glVertex3i(size, 0, size);
		glEnd();
	}

	if (isLightingEnabled) glEnable(GL_LIGHTING);
	glPopMatrix();
}

// funkce pro zpracovani klavesnice
void UpdateStepAnimation(float deltaTime) {
	if (isStepping) {
		stepTime += deltaTime;

		if (stepTime < stepDuration / 2) {
			trany = stepHeight * (stepTime / (stepDuration / 2));
		}
		else if (stepTime < stepDuration) {
			trany = stepHeight * ((stepDuration - stepTime) / (stepDuration / 2));
		}
		else {
			isStepping = false;
			stepTime = 0.0f;
			trany = 0.0f;
		}
	}
}

// funkce pro vykresleni bitmapoveho textu
void RenderBitmapString(float x, float y, void* font, const char* string) {
	glRasterPos2f(x, y);
	while (*string) {
		glutBitmapCharacter(font, *string++);
	}
}

// funkce pro vykresleni mraku
void drawCloud(float x, float y, float z)
{
	glPushMatrix();
	glTranslatef(x, y, z);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glEnable(GL_COLOR_MATERIAL);

	glColor4f(1.0f, 1.0f, 1.0f, 0.6f); 

	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.0f);
	gluSphere(quad, 6.0f, 20, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.5f, 0.0f, 0.0f);
	gluSphere(quad, 4.0f, 20, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.5f, 0.0f, 0.0f);
	gluSphere(quad, 4.0f, 20, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.0f, 2.0f, -1.0f);
	gluSphere(quad, 3.5f, 20, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.0f, 1.8f, 1.0f);
	gluSphere(quad, 3.5f, 20, 20);
	glPopMatrix();

	gluDeleteQuadric(quad);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glPopMatrix();
}

// funkce pro vykresleni mraku v kruhu
void drawCloudRing(int numClouds, float radius, float yHeight) {
	for (int i = 0; i < numClouds; ++i) {
		float angle = (2.0f * PI * i) / numClouds;
		float x = radius * cos(angle);
		float z = radius * sin(angle);
		drawCloud(x, yHeight, z);
	}
}

// funkce pro vykresleni fraktaloveho stromu
void drawFractalTree(float length, int iteration, float angle)
{
	if (iteration == 0)
		return;

	length = 25.f;

	glColor3f(0.5f, 0.35f, 0.05f);
	glLineWidth(iteration);

	glBegin(GL_LINES);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(0.0f, length);
	glEnd();

	glTranslatef(0.0f, length, 0.0f);
	glRotatef(angle, 0.0f, 0.0f, 1.0f);
	drawFractalTree(length * 0.7f, iteration - 1, angle);
	glRotatef(-2 * angle, 0.0f, 0.0f, 1.0f);
	drawFractalTree(length * 0.7f, iteration - 1, angle);
	glRotatef(angle, 0.0f, 0.0f, 1.0f);
	glTranslatef(0.0f, -length, 0.0f);
}

//vykresleni fraktaloveho stromu co se toci dle pozice kamery 
void drawSingleFractalTree(float treeX, float treeZ, float length, int iterations, float angle)
{
	glPushMatrix();
	glTranslatef(treeX, -15.0f, treeZ);

	if (enableTreeBillboarding && !enableRotation) {
		float camX = tranx;
		float camZ = tranz;
		float dx = camX - treeX;
		float dz = camZ - treeZ;
		float rotationY = atan2(dx, dz) * (180.0f / PI);
		glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
	}
	else if (enableRotation) {
		glRotatef(treeSpinOffset, 0.0f, 1.0f, 0.0f);
	}

	drawFractalTree(length, iterations, angle);
	glPopMatrix();
}

// 4 stromy ve tvaru ctverce -- "sad"
void drawFourFractalTrees(float spacing, float length, int iterations, float angle)
{
	float half = spacing / 2.0f;
	float positions[4][2] = {
		{-half, -half},  
		{-half,  half},  
		{ half, -half},  
		{ half,  half}   
	};
	for (int i = 0; i < 4; ++i)
	{
		float treeX = positions[i][0];
		float treeZ = positions[i][1];

		drawSingleFractalTree(treeX, treeZ, length, iterations, angle);
	}
}

// kontrolni body pro bezierovu plochu
GLfloat controlPoints[4][4][3] = {
	{{-1., -1.5, -2.0}, {-0.5, -1.5, 0.0}, {0.5, -1.5, 0.0}, {1.5, -1.5, -2.0}},
	{{-1., -0.5, 0.0}, {-0.5, -0.5, 4.0}, {0.5, -0.5, 4.0}, {1.5, -0.5, -0.0}},
	{{-1., 0.5, 0.0}, {-0.5, 0.5, 4.0}, {0.5, 0.5, 4.0}, {1.5, 0.5, 0.0}},
	{{-1., 1.5, -2.0}, {-0.5, 1.5, 0.0}, {0.5, 1.5, 0.0}, {1.5, 1.5, -2.0}}
};

// funkce pro vykresleni bezierovy plochy
void renderBezierSurface(float x, float y, float z) {
	int u, v;

	glPushMatrix(); 
	glTranslatef(x, -15.0f, z); 
	glScalef(38.0f, 38.0f, 38.0f); 
	glRotatef(-90., 1., 0., 0.);
	glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &controlPoints[0][0][0]);

	glEnable(GL_MAP2_VERTEX_3);
	glEnable(GL_TEXTURE_2D); 
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 

	for (u = 0; u < 30; u++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (v = 0; v <= 30; v++) {
			GLfloat color[4];
			color[0] = u / 30.0;
			color[1] = v / 30.0;
			color[2] = 1.0 - (u / 30.0);
			color[3] = 1.0f;
			glColor4fv(color); 
			glEvalCoord2f(u / 30.0, v / 30.0);
			glEvalCoord2f((u + 1) / 30.0, v / 30.0);
		}
		glEnd();
	}

	glDisable(GL_TEXTURE_2D); 
	glPopMatrix(); 
}

// funkce pro vykresleni koule/slunce
void drawSunSphere(float radius, int slices, int stacks)
{
	glColor3f(1.0f, 0.9f, 0.0f); 

	for (int i = 0; i < stacks; ++i) {
		float lat0 = PI * (-0.5 + (float)i / stacks);
		float lat1 = PI * (-0.5 + (float)(i + 1) / stacks);

		float y0 = sin(lat0);
		float y1 = sin(lat1);

		float r0 = cos(lat0);
		float r1 = cos(lat1);

		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= slices; ++j) {
			float lng = 2 * PI * (float)(j) / slices;
			float x = cos(lng);
			float z = sin(lng);

			glVertex3f(radius * x * r0, radius * y0, radius * z * r0);
			glVertex3f(radius * x * r1, radius * y1, radius * z * r1);
		}
		glEnd();
	}
}

// funkce pro vykresleni hlavy/krychle
void drawCube(float size)
{
	float hs = size / 2.0f;
	glBegin(GL_QUADS);

	// Oblicej
	glVertex3f(-hs, -hs, hs);
	glVertex3f(hs, -hs, hs);
	glVertex3f(hs, hs, hs);
	glVertex3f(-hs, hs, hs);

	// Tyl
	glVertex3f(-hs, -hs, -hs);
	glVertex3f(-hs, hs, -hs);
	glVertex3f(hs, hs, -hs);
	glVertex3f(hs, -hs, -hs);

	// Temeno
	glVertex3f(-hs, hs, -hs);
	glVertex3f(-hs, hs, hs);
	glVertex3f(hs, hs, hs);
	glVertex3f(hs, hs, -hs);

	// Brada
	glVertex3f(-hs, -hs, -hs);
	glVertex3f(hs, -hs, -hs);
	glVertex3f(hs, -hs, hs);
	glVertex3f(-hs, -hs, hs);

	// Lucho
	glVertex3f(-hs, -hs, -hs);
	glVertex3f(-hs, -hs, hs);
	glVertex3f(-hs, hs, hs);
	glVertex3f(-hs, hs, -hs);

	// Rucho
	glVertex3f(hs, -hs, -hs);
	glVertex3f(hs, hs, -hs);
	glVertex3f(hs, hs, hs);
	glVertex3f(hs, -hs, hs);
	glEnd();
}

// funkce pro vykresleni srdce postavy
void drawHeart()
{
	glPushMatrix();

	glTranslatef(2.0f, 4.0f, 1.0f);

	GLboolean lightingEnabled;
	glGetBooleanv(GL_LIGHTING, &lightingEnabled);
	if (lightingEnabled) glDisable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);

	glColor4f(1.0f, 0.0f, 0.0f, 1.0f); 

	float heartSize = 2.0f;
	drawCube(heartSize);

	glDisable(GL_BLEND);

	if (lightingEnabled) glEnable(GL_LIGHTING);

	glPopMatrix();
}

// funkce pro vykresleni postavy
void drawCharacter()
{
	glPushMatrix();

	// Srdce
	glPushMatrix();
	drawHeart();
	glPopMatrix();

	// Transparent
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	//  Thorax
	glColor4f(0.2f, 0.2f, 1.0f, 0.5f);
	glPushMatrix();
	glScalef(1.0f, 1.5f, 0.5f);
	drawCube(10.0f);
	glPopMatrix();

	//  Lruka
	glColor4f(0.8f, 0.1f, 0.1f, 0.4f);
	glPushMatrix();
	glTranslatef(-7.0f, 3.0f, 0.0f);
	glScalef(0.5f, 1.5f, 0.5f);
	drawCube(5.0f);
	glPopMatrix();

	//  Pruka
	glColor4f(0.8f, 0.1f, 0.1f, 0.4f);
	glPushMatrix();
	glTranslatef(7.0f, 3.0f, 0.0f);
	glScalef(0.5f, 1.5f, 0.5f);
	drawCube(5.0f);
	glPopMatrix();

	// Depthmaska
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	//  Hlava
	glPushMatrix();
	glTranslatef(0.0f, 10.0f, 0.0f);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, headTextureID);
	glColor3f(1.0f, 1.0f, 1.0f); 

	GLUquadric* quad = gluNewQuadric();
	gluQuadricTexture(quad, GL_TRUE);
	gluSphere(quad, 4.0, 20, 20);
	gluDeleteQuadric(quad);

	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	//  Lnoha
	glColor3f(0.1f, 0.5f, 0.1f);
	glPushMatrix();
	glTranslatef(-2.5f, -10.0f, 0.0f);
	glScalef(0.5f, 1.5f, 0.5f);
	drawCube(5.0f);
	glPopMatrix();

	//  Rnoha
	glPushMatrix();
	glTranslatef(2.5f, -10.0f, 0.0f);
	glScalef(0.5f, 1.5f, 0.5f);
	drawCube(5.0f);
	glPopMatrix();

	glPopMatrix();
}


// funkce pro kontrolu kolize s telem postavy
void checkCollisionWithCharacterBody()
{
	const float bodyX = 0.0f;
	const float bodyY = 0.0f;
	const float bodyZ = 50.0f;

	const float halfWidth = 5.0f; 
	const float halfHeight = 14.0f; 
	const float halfDepth = 2.5f;  

	bool insideX = tranx > bodyX - halfWidth && tranx < bodyX + halfWidth;
	bool insideY = trany > bodyY - halfHeight && trany < bodyY + halfHeight;
	bool insideZ = tranz > bodyZ - halfDepth && tranz < bodyZ + halfDepth;

	if (insideX && insideY && insideZ)
	{
		lastCommand = "Collision with character body!";
		tranx = tranx_prev;
		trany = trany_prev;
		tranz = tranz_prev;
	}
}

void OnDisplay(void)
{
	static float lastTime = 0.0f;
	float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	float deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	UpdateStepAnimation(deltaTime);
	if (enableRotation) {
		rOffset += deltaTime * 20.0f;
		if (rOffset > 360.0f) rOffset -= 360.0f;

		treeSpinOffset += deltaTime * 45.0f;
		if (treeSpinOffset > 360.0f) treeSpinOffset -= 360.0f;
	}


	GLfloat lightPosition[] = { 0.0f, 200.0f, 0.0f, 1.0f }; 
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	glClearColor(0.8, 0.8, 0.8, 0.0);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	
	glRotatef(-180 * ynew / PI, 1.0f, 0.0f, 0.0f);
	glRotatef(180 * xnew / PI, 0.0f, 1.0f, 0.0f);
	glTranslatef(tranx, -trany, tranz);

	DrawPlane(250);
	
	glPushMatrix();
	glTranslatef(0.0f, 200.0f, 0.0f);  
	drawSunSphere(10.0f, 30, 30);        
	glPopMatrix();

	drawFourFractalTrees(120.0f, 8.0f, 5, 30);

	glPushMatrix();
	drawCloudRing(10, 80.0f, 100.0f); 
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 14.0f, -50.0f); 
	glScalef(40.0f, 40.0f, 40.0f);      

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, vodkaTexture);

	for (auto& mesh : vodkaLoader.LoadedMeshes)
	{
		glBegin(GL_TRIANGLES);
		for (unsigned int i : mesh.Indices)
		{
			objl::Vertex v = mesh.Vertices[i];

			glNormal3f(v.Normal.X, v.Normal.Y, v.Normal.Z);
			glTexCoord2f(v.TextureCoordinate.X, v.TextureCoordinate.Y);
			glVertex3f(v.Position.X, v.Position.Y, v.Position.Z);
		}
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
	glPopMatrix();


	int numObjects = 15;
	float radius = (750.0f + xOffset) / 3.0f;
	glPushMatrix();
	glRotatef(rOffset, 0.0f, 1.0f, 0.0f);
	for (int i = 0; i < numObjects; i++) {
		float angle = (2.0f * PI * i) / numObjects;
		float x = radius * cos(angle);
		float z = radius * sin(angle);
		renderBezierSurface(x, 0.0f, z);
	}
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -50.0f);
	drawCharacter();
	glPopMatrix();

	GLboolean lightingWasEnabled, textureWasEnabled;
	glGetBooleanv(GL_LIGHTING, &lightingWasEnabled);
	glGetBooleanv(GL_TEXTURE_2D, &textureWasEnabled);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, w, 0, h);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glColor3f(0.0f, 0.0f, 0.0f);
	RenderBitmapString(10, h - 20, GLUT_BITMAP_HELVETICA_18, lastCommand.c_str());

	std::string coordX = "X: " + std::to_string(tranx);
	std::string coordY = "Y: " + std::to_string(trany);
	std::string coordZ = "Z: " + std::to_string(tranz);

	int offsetX = w - 150;  
	int baseY = h - 20;     

	RenderBitmapString(offsetX, baseY, GLUT_BITMAP_HELVETICA_18, coordX.c_str());
	RenderBitmapString(offsetX, baseY - 20, GLUT_BITMAP_HELVETICA_18, coordY.c_str());
	RenderBitmapString(offsetX, baseY - 40, GLUT_BITMAP_HELVETICA_18, coordZ.c_str());

	std::string staminaText = "Stamina: " + std::to_string((int)stamina);
	RenderBitmapString(offsetX, baseY - 60, GLUT_BITMAP_HELVETICA_18, staminaText.c_str());

	buttonWidth = 160;
	buttonHeight = 40;
	buttonX = w - buttonWidth - 20;
	buttonY = 20;

	glColor3f(buttonClicked ? 0.4f : 0.6f, 0.6f, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(buttonX, buttonY);
	glVertex2f(buttonX + buttonWidth, buttonY);
	glVertex2f(buttonX + buttonWidth, buttonY + buttonHeight);
	glVertex2f(buttonX, buttonY + buttonHeight);
	glEnd();

	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(buttonX, buttonY);
	glVertex2f(buttonX + buttonWidth, buttonY);
	glVertex2f(buttonX + buttonWidth, buttonY + buttonHeight);
	glVertex2f(buttonX, buttonY + buttonHeight);
	glEnd();

	glColor3f(0.0f, 0.0f, 0.0f);
	RenderBitmapString(buttonX + 8, buttonY + 12, GLUT_BITMAP_HELVETICA_18, "Refill stamina click");

	if (lightingWasEnabled) glEnable(GL_LIGHTING);
	if (textureWasEnabled) glEnable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glFlush();
	glutSwapBuffers();
}

void OnMouseButton(int button, int state, int x, int y)
{
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (x >= buttonX && x <= buttonX + buttonWidth &&
			y >= buttonY && y <= buttonY + buttonHeight) {
			buttonClicked = true;
			stamina = maxStamina; 
			lastCommand = "Stamina refilled!";
		}
		else {
			buttonClicked = false;
		}

		tocime = true;
		xx = x;
		yy = y;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		tocime = false;
		xold = xnew;
		yold = ynew;
	}

	// pripadne test pro prave tlacitko pro vyvolani menu
	// ... GLUT_RIGHT_BUTTON

	glutPostRedisplay();
}

// funkce volana pri pohybu mysi a stistenem tlacitku mysi
void OnMouseMotionPassive(int x, int y) {

	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	float deltaX = x - xx;
	float deltaY = y - yy;

	xnew += deltaX / radius;
	ynew += deltaY / radius;

	if (ynew > MPI4) {
		ynew = MPI4;  
	}
	if (ynew < -MPI4) {
		ynew = -MPI4;  
	}

	if (xnew > 2 * PI) {
		xnew -= 2 * PI;  
	}
	if (xnew < 0) {
		xnew += 2 * PI;  
	}

	xx = x;
	yy = y;

	glutPostRedisplay();
}

//funkce pro obsluhu klaves
void OnSpecial(int key, int mx, int my)
{
	float speed = 4.0f;
	float yaw = xnew;
	float pitch = ynew;

	tranx_prev = tranx;
	trany_prev = trany;
	tranz_prev = tranz;

	switch (key)
	{
	case GLUT_KEY_F1:
		MenuHandler(3);
		break;
	case GLUT_KEY_F2:
		MenuHandler(4);
		break;
	case GLUT_KEY_F3:
		MenuHandler(5);
		break;
	case GLUT_KEY_F4:
		MenuHandler(6);
		break;
	case GLUT_KEY_F5:
		MenuHandler(7);
		break;
	case GLUT_KEY_F6:
		MenuHandler(8);
		break;
	case GLUT_KEY_F7:
	{
		GLboolean isLightingEnabled = glIsEnabled(GL_LIGHTING);
		if (isLightingEnabled)
			LightingMenuHandler(2); // Turns OFF lighting
		else
			LightingMenuHandler(1); // Turns ON lighting
		break;
	}


	case GLUT_KEY_PAGE_UP:
		if (trany < 50.0f) {
			trany += 2.0f;
		}
		break;

	case GLUT_KEY_PAGE_DOWN:
		if (trany > -10.0f) {
			trany -= 2.0f;
		}
		break;

	case GLUT_KEY_UP:
		if (stamina <= 0) {
			lastCommand = "Too tired to move!";
			break;
		}
		if (enableStepping) isStepping = true;
		tranx -= sin(yaw) * cos(pitch) * speed;
		tranz += cos(yaw) * cos(pitch) * speed;
		trany += sin(pitch) * speed;

		checkCollisionWithCharacterBody();
		if (tranx != tranx_prev || trany != trany_prev || tranz != tranz_prev) {
			stamina--;
			lastCommand = "Moving FORWARDS";
		}
		else {
			lastCommand = "Bumped into character!";
		}
		break;

	case GLUT_KEY_DOWN:
		if (stamina <= 0) {
			lastCommand = "Too tired to move!";
			break;
		}
		if (enableStepping) isStepping = true;
		tranx += sin(yaw) * cos(pitch) * speed;
		tranz -= cos(yaw) * cos(pitch) * speed;
		trany -= sin(pitch) * speed;

		checkCollisionWithCharacterBody();
		if (tranx != tranx_prev || trany != trany_prev || tranz != tranz_prev) {
			stamina--;
			lastCommand = "Moving BACKWARDS";
		}
		else {
			lastCommand = "Bumped into character!";
		}
		break;

	case GLUT_KEY_LEFT:
		if (stamina <= 0) {
			lastCommand = "Too tired to move!";
			break;
		}
		if (enableStepping) isStepping = true;
		tranx += cos(yaw) * speed;
		tranz += sin(yaw) * speed;

		checkCollisionWithCharacterBody();
		if (tranx != tranx_prev || trany != trany_prev || tranz != tranz_prev) {
			stamina--;
			lastCommand = "Moving LEFT";
		}
		else {
			lastCommand = "Bumped into character!";
		}
		break;

	case GLUT_KEY_RIGHT:
		if (stamina <= 0) {
			lastCommand = "Too tired to move!";
			break;
		}
		if (enableStepping) isStepping = true;
		tranx -= cos(yaw) * speed;
		tranz -= sin(yaw) * speed;

		checkCollisionWithCharacterBody();
		if (tranx != tranx_prev || trany != trany_prev || tranz != tranz_prev) {
			stamina--;
			lastCommand = "Moving RIGHT";
		}
		else {
			lastCommand = "Bumped into character!";
		}
		break;
	}

	glutPostRedisplay();
}

// funkce volana pro klavesnici
void OnKeyboard(unsigned char key, int x, int y)
{
	float speed = 2.0f;
	float yaw = xnew;
	float pitch = ynew;

	switch (key)
	{
	case 27:
		MenuHandler(0);
		break;
	}
	glutPostRedisplay();
}

// funkce main
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);                    // inicializace knihovny GLUT
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH); // init double buffer

	glutInitWindowSize(640, 480);            // nastaveni pocatecni velikosti dale oteviranych oken
	glutInitWindowPosition(200, 200);        // nastaveni pocatecniho umisteni dale oteviranych oken

	glutCreateWindow("Projekt 2025 – Autor: 237853 Baranyk Matej");    // vytvoreni okna

	// registracni funkce
	glutDisplayFunc(OnDisplay);                // registrace funkce volane pri prekreslovani aktualniho okna
	glutReshapeFunc(OnReshape);                // registrace funkce volane pri zmene velikosti aktualniho okna
	glutMouseFunc(OnMouseButton);            // registrace funkce pro stisk tlacitka mysi
	glutPassiveMotionFunc(OnMouseMotionPassive);            // registrace funkce pro pohyb mysi pri stisknutem tlacitku
	glutSpecialFunc(OnSpecial);                // registrace funkce pro zachytavani specialnich klaves
	glutKeyboardFunc(OnKeyboard);            // registrace funkce pro zachytavani stisku klaves
	glutIdleFunc(OnIdle); // Add this in main()

	// pripadne dalsi udalosti...

	// inicializace sceny
	OnInit();

	glutMainLoop();

	return 0;
}
