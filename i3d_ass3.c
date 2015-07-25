/* Name:Li Zuyi
* Sid: s3260309
* Account no: s3260309
* i3d Assignment 3, 2011
* $Id$
*/

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texture.h"
#include "obj.h"
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glut.h>
#include "texture.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define clamp(x, a, b) min(max(x, a), b)

#define WINDOW_TITLE "I3D Assignment 3"

enum RenderOptions {
  OPT_WIREFRAME,
  OPT_DRAW_AXES,
  OPT_ORTHOGRAPHIC,
  OPT_OFF_LIGHT,
  OPT_OFF_CULL,
  OPT_BACK_FRONT_CULL,
  OPT_STOP_ANIMATION,
  OPT_RENDER_SATURN,
  OPT_RENDER_RING,
  OPTSIZE //NOTE: this must be the last enumerator.
};

typedef struct Vec3f {
  float x, y, z;
} Vec3f;

typedef struct Vec2f {
  float u, v;
} Vec2f;

typedef struct Location {
  float x, y,z;
} Location;

typedef struct Camera {
  Vec3f rot; // Euler angles - pitch/yaw/roll.
  Vec3f pos;
  float zoom;
  float sensitivity;
  float fov;
  bool rotating;
  bool zooming;
} Camera;

typedef struct Mesh {
  Vec3f** grid;
  Vec3f** normal;
  int rows;
  int cols;
} Mesh;

static Mesh   saturn;
static Mesh   rings;
static Mesh   particle;
static Camera camera;
static bool   options[OPTSIZE];
static int    primitiveMode      = 0;
static int    tessellation       = 8;
static float  saturnTilt         = 0.0f;
static float  animationSpeed     = 10;
static const float clipFar       = (3560000 * 2.1);
static const float clipNear      = (3560000 * 2.1) / 10000.0;
static const float saturnHour    = 10.3235/24;
static const float saturnYear    = 10.3235/24/10832;
static float       currTime      = 0.0 ,startTime=0.0, endTime = 0.0,frameTime = 0.0;
static int         particleCount = 0;
static const int   ms            = 1000;
static const int   bufsize       = 30;


#define DRAW_SATURN         0
#define DRAW_PARTICLE       0
#define DRAW_RING           1
#define SATURN_TESS         8
#define PARTICLE_TESS       8
#define SATURN_RADIUS       60268
#define RING_DIST_INNER     89000
#define RING_DIST_OUTER     140390
#define RING_NORMAL         10000
#define SATURN_TILT_MAX     27
#define SATURN_SCALE_HEIGHT 0.90200438 // Squash saturn by 10%.
#define DEFAULT_LENGTH      500
#define MAX_DEG             360
#define GL_PARTICLE         3
#define POINT_SIZE          2.0f
#define PARTICLE_RAD        120.0f
#define MAX_TESS            2048
#define MAX_SPPED           1000
float saturnTheta         = 0.0;
float saturnYearTheta     = 0.0;
int ringTess              = 4;

int buttons[5] ={GLUT_UP,GLUT_UP,GLUT_UP,GLUT_UP,GLUT_UP};
float panX = 0 ,panY = 0, panZ = 0;

//#define DEBUG_MESH
//#define DEBUG_CAMERA

static const GLenum primitiveTypes[] = {GL_QUADS,GL_POINTS,GL_PARTICLE};

#define checkGLErrors() _checkGLErrors(__LINE__)
void _checkGLErrors(int line)
{
  GLuint err;
  if ((err = glGetError()) != GL_NO_ERROR)
    fprintf(stderr, "OpenGL error caught on line %i: %s\n", line, gluErrorString(err));
}

void drawAxes(float len, int lineWidth)
{
  //  if (!options[OPT_DRAW_AXES])
  //    return;
  glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);

  glLineWidth(lineWidth);
  glBegin(GL_LINES);
  glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(len, 0, 0);
  glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, len, 0);
  glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, len);
  glColor3f(0.701, 0.56, 0.35);
  glEnd();
  glPopAttrib();
}

void allocateMesh(Mesh* mesh)
{
  // Allocate grid.
  mesh->grid = (Vec3f**)malloc(sizeof(Vec3f*) * mesh->rows);  // Allocate array of pointers.
  mesh->normal = (Vec3f**)malloc(sizeof(Vec3f*) * mesh->rows);// Allocate array of pointers.
  for (int i = 0; i < mesh->rows; i++)
  {
    mesh->grid[i] = (Vec3f*)malloc(sizeof(Vec3f)  * mesh->cols); // Allocate arrays of data.
    mesh->normal[i] = (Vec3f*)malloc(sizeof(Vec3f) * mesh->cols);// Allocate arrays of data.
  }
}

void createSphere(Mesh *mesh, int cols, int rows, float r)
{
#ifdef DEBUG_MESH
  fprintf(stderr, "new sphere %i %i\n", cols, rows);
#endif
  // Allocate a mesh to return.
  mesh->rows = rows;
  mesh->cols = cols;
  allocateMesh(mesh);

  // Create the vertices for a sphere.
  for (int i = 0; i < mesh->rows; ++i)
  {
    float v = M_PI * (float)i / (mesh->rows-1);
    for (int j = 0; j < mesh->cols; ++j)
    {
      float u = 2.0 * M_PI * (float)j / (mesh->cols-1);

      // Parametric coordinates.
      //http://en.wikipedia.org/wiki/Parametric_surface
      mesh->grid[i][j].x = r * cos(u) * sin(v);
      mesh->grid[i][j].y = r * sin(u) * sin(v);
      mesh->grid[i][j].z = r * cos(v);

      mesh->normal[i][j].x = cos(u) * sin(v);
      mesh->normal[i][j].y = sin(u) * sin(v);
      mesh->normal[i][j].z = cos(v);

    }
  }
}


void createDisc(Mesh *mesh, int cols, int rows, float r, float R)
{
#ifdef DEBUG_MESH
  fprintf(stderr, "new disc %i %i\n", cols, rows);
#endif
  // Allocate a mesh to return.
  mesh->rows = rows;
  mesh->cols = cols;
  allocateMesh(mesh);

  // Create the vertices for a disc.
  float d = R - r;

  for (int i = 0; i < mesh->rows; ++i)
  {
    float v = (float)i / (mesh->rows-1);
    for (int j = 0; j < mesh->cols; ++j)
    {
      float u = 2.0 * M_PI * (float)j / (mesh->cols-1);

      mesh->grid[i][j].x = cos(u) * (r + d * v);
      mesh->grid[i][j].y = sin(u) * (r + d * v);
      mesh->grid[i][j].z = 0.0;

      mesh->normal[i][j].x = mesh->grid[i][j].x;
      mesh->normal[i][j].y = mesh->grid[i][j].y;
      mesh->normal[i][j].z = RING_NORMAL;

    }
  }
  particleCount = (rows - 1) * (cols - 1);
}

void drawMesh(Mesh* mesh,int which_to_draw)
{
  GLuint primitiveType;
  glEnable(GL_NORMALIZE);
  glEnable(GL_RESCALE_NORMAL);
  if (options[OPT_DRAW_AXES])
    drawAxes(2.0, 1);

  if(which_to_draw == DRAW_SATURN)
    primitiveType = GL_QUADS;
  else
    primitiveType = primitiveTypes[primitiveMode]; // Draws each row of the mesh using the set primitive type.

  bool strips = (primitiveType == GL_QUAD_STRIP || primitiveType == GL_TRIANGLE_STRIP);

  for (int i = 0; i < mesh->rows - 1; ++i)
  {
    glBegin(primitiveType);

    for (int j = 0; j < mesh->cols - 1; ++j)
    {
      const Vec3f a = mesh->grid[i][j];
      const Vec3f b = mesh->grid[i+1][j];
      const Vec3f t = mesh->normal[i][j];
      const Vec3f y = mesh->normal[i+1][j];

      glNormal3fv((GLfloat*)&t);
      glVertex3f(a.x, a.y, a.z); // Always draw the first two vertices at this iteration.

      glNormal3fv((GLfloat*)&y);
      glVertex3f(b.x, b.y, b.z);
      if (!strips)
      {
        // If using quads, another two vertices are needed.
        const Vec3f c = mesh->grid[i][j+1];
        const Vec3f d = mesh->grid[i+1][j+1];
        const Vec3f p = mesh->normal[i][j+1];
        const Vec3f q = mesh->normal[i+1][j+1];

        glNormal3fv((GLfloat*)&q);
        glVertex3f(d.x, d.y, d.z);
        glNormal3fv((GLfloat*)&p);
        glVertex3f(c.x, c.y, c.z);
      }
    }

    glEnd();
  }
}

void drawEPM(Mesh* mesh)
{
  glPushMatrix();
  glPushAttrib(GL_CURRENT_BIT);
  glColor3f(1, 1, 0);
  glLineWidth(2);

  // Equator
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < mesh->rows; ++i)
  {
    const int j = mesh->cols/2;
    const Vec3f a = mesh->grid[i][j];
    glVertex3f(a.x, a.y, a.z);
  }
  glEnd();

  // Prime Meridian
  glBegin(GL_LINE_STRIP);
  for (int j = 0; j < mesh->cols; ++j)
  {
    const int i = mesh->rows/2;
    const Vec3f a = mesh->grid[i][j];
    glVertex3f(a.x, a.y, a.z);
  }
  glEnd();

  glPopAttrib();
  glPopMatrix();
  glLineWidth(1);
}

void freeMesh(Mesh* mesh)
{
  // Free allocated data.
  if (mesh->grid)
  {
    for (int i = 0; i < mesh->rows; ++i) {
      free(mesh->grid[i]);
      free(mesh->normal[i]);
    }
    free(mesh->grid);
    free(mesh->normal);
  }
  mesh->grid   = NULL;
  mesh->normal = NULL;
  mesh->rows = 0;
  mesh->cols = 0;
}

void updateRenderState()
{
  if(options[OPT_BACK_FRONT_CULL])
    glCullFace(GL_FRONT);
  else
    glCullFace(GL_BACK);

  // Set wireframe state.
  if (options[OPT_WIREFRAME])
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}

void createGeometry()
{
  // Free the meshes (if they exist).
  freeMesh(&saturn);
  freeMesh(&rings);
  freeMesh(&particle);

  int tessSaturn = tessellation;

  // Create the unique meshes and make sure the tessellation is not below 4.
  // Note that scaling is applied before drawing in order to reuse meshes.
  createSphere(&saturn, SATURN_TESS+1, SATURN_TESS+1, 1.0);
  createSphere(&particle,PARTICLE_TESS+1,PARTICLE_TESS+1,PARTICLE_RAD);
//  createDisc(&rings, tessSaturn+1, max(4, tessSaturn/4)+1, RING_DIST_INNER, RING_DIST_OUTER);
  createDisc(&rings, ringTess+1, ringTess+1,RING_DIST_INNER, RING_DIST_OUTER);
}

void resetCamara()
{
  camera.rot.x = 45.0f; camera.rot.y = 0.0f; camera.rot.z = 0.0f;
  camera.pos.x = 0.0f; camera.pos.y = 0.0f; camera.pos.z = 0.0f;
  camera.zoom = SATURN_RADIUS * 4.0f;
  camera.rotating = camera.zooming = false;
  camera.sensitivity = 0.4;
  camera.fov = 60.0;
}

void init()
{
  // Initialize options array.
  memset(options, 0, sizeof(bool) * OPTSIZE);

  // Initialize camera.
  camera.rot.x = 45.0f; camera.rot.y = 0.0f; camera.rot.z = 0.0f;
  camera.pos.x = 0.0f; camera.pos.y = 0.0f; camera.pos.z = 0.0f;
  camera.zoom = SATURN_RADIUS * 4.0f;
  camera.rotating = camera.zooming = false;
  camera.sensitivity = 0.4;
  camera.fov = 60.0;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLfloat diffuseMaterial[4] = { 1, 1, 1, 0.1 };
  GLfloat mat_ambient[] = { 0.2, 0.2, 0.2, 0.01 };
  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
  GLfloat light_ambient[] = { 1, 1, 1, 0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 0.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1 };
  glClearColor (0.0, 0.0, 0.0, 0.0);

  glShadeModel (GL_SMOOTH);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glColorMaterial(GL_FRONT, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glPointSize(POINT_SIZE);
  // Initialize geometry.
  createGeometry();
}

void reshape(int x, int y)
{
  // Set the portion of the window to draw to.
  glViewport(0, 0, x, y);
  // Modify the projection matrix.
  glMatrixMode(GL_PROJECTION);
  // Clear previous projection.
  glLoadIdentity();
  // Create new projection.
  float aspect = x / (float)y;
  if(options[OPT_ORTHOGRAPHIC])
  {
    float d = clipNear + sqrt(clipFar - clipNear);
    glOrtho(-d * 0.5, d * 0.5, - d * 0.5 / aspect,
             d * 0.5 / aspect, clipNear, clipFar);
  }
  else
    gluPerspective(camera.fov, aspect, clipNear, clipFar);
  //IMPORTANT: go back to modifying the modelview matrix.
  glMatrixMode(GL_MODELVIEW);
}

void setupCamera()
{
  // Handle orthographic projection "zooming".
  if(options[OPT_ORTHOGRAPHIC])
  {
    float s = 1000.0/((camera.zoom - clipNear)*tan(camera.fov));
    glTranslatef(0, 0, -clipFar/2.0);
    glScalef(s, s, 1.0);
  }
  else
    glTranslatef(0, 0, -camera.zoom);

  // Inverse camera transform.
#if 1
  glTranslatef(-camera.pos.x, -camera.pos.y, -camera.pos.z);
#endif
#if 0
  gluLookAt(0,0,-camera.zoom,
            panX,panY,0,
            0.00,1.00,0.00); 
#endif
  // Do panning first than rotate  
  glTranslatef(panX,panY,panZ);
  glRotatef(camera.rot.x, 1, 0, 0);
  glRotatef(camera.rot.y, 0, 1, 0);

}

void drawParticles(Mesh* mesh)
{
  glPushMatrix();

  for (int i =0; i< mesh->rows; ++i)
  {
    for(int j = 0; j < mesh->cols; ++j)
    { 
      const Vec3f p = mesh->grid[i][j];
      glPushMatrix();
      glTranslatef(p.x,p.y,p.z);
      drawMesh(&particle,DRAW_PARTICLE);
      glPopMatrix();
    }
  }
  glPopMatrix();

}

void drawEntity(int entity)
{
  const float lightBrown[] = {224.0/255.0, 188.0/255.0, 126.0/255.0};
  const float darkBrown[]  = {120.0/255.0, 108.0/255.0, 86.0/255.0};
  GLuint primitiveType = primitiveTypes[primitiveMode];
  // Draw saturn.
  glPushMatrix();

  glScalef(1.0, 1.0, SATURN_SCALE_HEIGHT);
  // Rotate saturn acording to saturn time
  glRotatef(saturnTheta, 0, 0, 1);

  // Reset to 0 when reach 360 deg
  if(saturnTheta > MAX_DEG)
    saturnTheta -=MAX_DEG;
  if (!options[OPT_WIREFRAME])
    glColor3fv(lightBrown);

  if(!options[OPT_RENDER_SATURN]) {
    glPushMatrix();

    glScalef(SATURN_RADIUS, SATURN_RADIUS, SATURN_RADIUS);
    drawMesh(&saturn,DRAW_SATURN);
    // Draw equator and meridian for Saturn.
    if (options[OPT_WIREFRAME])
      drawEPM(&saturn);

    glPopMatrix();

    // Color wireframe
    if (!options[OPT_WIREFRAME])
      glColor3fv(darkBrown);
  }

  if(!options[OPT_RENDER_RING]) {
    if(primitiveType == GL_PARTICLE)
      drawParticles(&rings);          // Draw the particles.
    else
      drawMesh(&rings,DRAW_RING);     // Draw the rings.
  }

  glPopMatrix();

}

void displayPerformanceInfo()
{
  static float       prevTime              = 0.0;
  static int         prevFrameNo           = 0;
  static int         currFrameNo           = 0;
  static float       fps                   = 0.0;
  static const float timeInterval          = 1.0;
  static char        fpsBuffer     [30];
  static char        ftBuffer      [30];
  static char        particleBuffer[30];
  int len, i;

  // Calc time frame
  endTime   = glutGet(GLUT_ELAPSED_TIME);
//frameTime = endTime - startTime;

  // Start counting the frames
  currFrameNo++;

  // Update fps in interval of 1 sec
  if(currTime > prevTime + timeInterval) {
   
    fps = (currFrameNo - prevFrameNo) /  (currTime - prevTime);
    frameTime = endTime - startTime; 
    prevTime = currTime;
    prevFrameNo = currFrameNo;
  }
 
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  // Remeber light setting
  glPushAttrib(GL_ENABLE_BIT);
  glPushMatrix();

  // Disable lighting
  glDisable(GL_LIGHTING);
  // Reset
  glLoadIdentity();

  // Create new projection.
  glOrtho(-1, 1, - 1,1, 1, -1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

 
  // Print out system performs info
  strcpy(particleBuffer,  "Particles:       ");
  sprintf(&particleBuffer[15], "      %d", particleCount);

  strcpy(fpsBuffer,       "Frame rate (fps):     ");
  sprintf(&fpsBuffer[18], "%5.0f", fps);

  strcpy(ftBuffer,        "Frame time (ms):     ");
  sprintf(&ftBuffer[18],  "%5.0f",  frameTime);

  len = (int)strlen(fpsBuffer);
  glRasterPos2f(-0.8, -0.7);

  for (i = 0; i < len; i++)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, fpsBuffer[i]);

  len = (int)strlen(ftBuffer);
  glRasterPos2f(-0.8, -0.75);

  for (i = 0; i < len; i++)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ftBuffer[i]);

  len = (int)strlen(particleBuffer);
    glRasterPos2f(-0.8, -0.8);

  for (i = 0; i < len; i++)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, particleBuffer[i]);
  
  // Return to original setting
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
}

void display()
{
  // Record start time
  startTime = glutGet(GLUT_ELAPSED_TIME);

  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Position the camera
  setupCamera();

  //  drawAxes(SATURN_RADIUS,1);
  //  glTranslatef(panX,panY,panZ);

  if(options[OPT_DRAW_AXES])
    drawAxes(SATURN_RADIUS*4.0, 2);

  glRotatef(-90.0 + saturnTilt, 1.0, 0.0, 0.0); // Tilt of planet.
  glColor3f(1.0, 1.0, 1.0);

  glEnable(GL_RESCALE_NORMAL);
  {   /* Draw saturn and its ring*/
    drawEntity(DRAW_SATURN);
  }

  /* Display frame rate and times. */
  currTime = glutGet(GLUT_ELAPSED_TIME) / (float)ms;

  displayPerformanceInfo();

  glutSwapBuffers();
  // Check for gl errors.
  checkGLErrors();
}

void idle()
{
  int now_time;
  static int last_time;
  float elapsed_time;

  if(!options[OPT_STOP_ANIMATION]) {
    now_time = glutGet(GLUT_ELAPSED_TIME);
    // converts to seconds
    elapsed_time = (now_time - last_time)*0.001;
    last_time = now_time;

    elapsed_time *= animationSpeed;
    saturnTheta +=saturnHour * elapsed_time;
    saturnYearTheta +=saturnYear*elapsed_time;

  }
  else {
    last_time = glutGet(GLUT_ELAPSED_TIME);
  }
  glutPostRedisplay();
}

void cleanup()
{
  freeMesh(&saturn);
  freeMesh(&rings);
}


void mouseDown(int button, int state, int x, int y)
{
#ifdef DEBUG_CAMERA
  printf("%d %d\n", camera.rotating, camera.zooming);
#endif

  buttons[button] = state;

  if(button == GLUT_LEFT_BUTTON)
    camera.rotating = state == GLUT_DOWN;
  if(button == GLUT_RIGHT_BUTTON)
    camera.zooming = state == GLUT_DOWN;
}

void mouseMove(int x, int y)
{
  static int prevX, prevY;

  int movX = x - prevX;
  int movY = y - prevY;

  float unitX;
  float unitY;
  // Get object space units
  if(options[OPT_ORTHOGRAPHIC]) {
    unitX = camera.zoom/glutGet(GLUT_WINDOW_HEIGHT);
    unitY = camera.zoom/glutGet(GLUT_WINDOW_HEIGHT);
  }
  else {
    unitX = camera.zoom/glutGet(GLUT_WINDOW_HEIGHT);
    unitY = camera.zoom/glutGet(GLUT_WINDOW_HEIGHT);
  }
  if(camera.rotating)
  {
    // Rotate the camera.
    camera.rot.x += movY * camera.sensitivity;
    camera.rot.y += movX * camera.sensitivity;

    // Clamp camera's pitch.
    camera.rot.x = clamp(camera.rot.x, -90.0, 90.0);
  }

  if(camera.zooming)
  {
    // Zoom in/out (speed is proportional to the current zoom).
    camera.zoom -= movY * camera.zoom * camera.sensitivity * 0.1;

    // Clamp the camera's zoom.
    camera.zoom = clamp(camera.zoom, clipNear, clipFar / 2.0);
  }

  if(buttons[GLUT_MIDDLE_BUTTON] == GLUT_DOWN)
  { // Scale dx and dy to object space coordinates
    panX += (movX * unitX);
    panY +=-(movY * unitY);
  }

  prevX = x;
  prevY = y;
}

void keyDown(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:
  case 'q':
    // Exit if esc or q is pressed.
    cleanup();
    exit(0);
    break;
  case 'w':
    options[OPT_WIREFRAME] = !options[OPT_WIREFRAME];
    break;
  case 'a':
    options[OPT_DRAW_AXES] = !options[OPT_DRAW_AXES];
    break;
  case 'p':
    resetCamara();
    options[OPT_ORTHOGRAPHIC] = !options[OPT_ORTHOGRAPHIC];
    reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    break;
  case '+':
  case '-':
    if(k == '+') {
      ringTess = min(MAX_TESS, ringTess * 2);
//      tessellation = min(MAX_TESS, tessellation * 2);
    }
    else {
      ringTess = max(4, ringTess / 2);
//      tessellation = max(4, tessellation / 2);
    }
    particleCount = 0;
    createGeometry();
    break;
  case '*':
  case '/':
    if (k == '*')
      animationSpeed = min(MAX_SPPED, animationSpeed * 2);
    else
      animationSpeed = max(10, animationSpeed / 2);
    break;
  case 'y':
  case 'Y':
    if (k == 'Y')
      saturnTilt += 1.0;
    else
    saturnTilt -= 1.0;
    saturnTilt = fmin(fmax(saturnTilt, 0.0), SATURN_TILT_MAX);
    break;
  case 'm': primitiveMode = (primitiveMode + 1) % (sizeof(primitiveTypes) / sizeof(GLenum));
    break;
  case 'l': options[OPT_OFF_LIGHT] = !options[OPT_OFF_LIGHT];
    if(options[OPT_OFF_LIGHT])
      glDisable(GL_LIGHTING);
    else
    glEnable(GL_LIGHTING);
    break;

  case 'c': options[OPT_OFF_CULL] = !options[OPT_OFF_CULL];
    if(options[OPT_OFF_CULL])
      glEnable(GL_CULL_FACE);
    else
    glDisable(GL_CULL_FACE);
    break;
  case 'f': options[OPT_BACK_FRONT_CULL] =!options[OPT_BACK_FRONT_CULL];
    break;
  case 'g': options[OPT_STOP_ANIMATION] =!options[OPT_STOP_ANIMATION];
    break;
  case 's': options[OPT_RENDER_SATURN] =!options[OPT_RENDER_SATURN];
    break;
  case 'r': options[OPT_RENDER_RING]   =!options[OPT_RENDER_RING];
    break;
  }
  updateRenderState();
}


int main(int argc, char **argv)
{
  // Init glut and create window.
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(1500,1000);
  glutCreateWindow(WINDOW_TITLE);
  glCullFace(GL_FRONT_AND_BACK);
  // Set glut defaults.
  glEnable(GL_DEPTH_TEST);
  // Set glut callbacks.
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutReshapeFunc(reshape);
  glutMotionFunc(mouseMove);
  glutPassiveMotionFunc(mouseMove);
  glutMouseFunc(mouseDown);
  glutKeyboardFunc(keyDown);

  // Initialize variables, create scene etc.
  init();
  glutMainLoop();

  return EXIT_SUCCESS;
}
