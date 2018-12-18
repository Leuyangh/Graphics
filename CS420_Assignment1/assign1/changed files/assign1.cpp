// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 480 Computer Graphics
  Assignment 1: Height Fields
  C++ starter code
*/



#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "CImg-2.3.5\CImg.h"
#include "pic.h"
using namespace cimg_library;

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;
//render state
typedef enum { WIREFRAME, VERTICES, TRIANGLE} RENDERSTATE;
RENDERSTATE RenderState = WIREFRAME;
//color choices
typedef enum { RED, BLUE, PURPLEPINK, GREEN} COLORSTATE;
COLORSTATE color = BLUE;
/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

CImg<unsigned char> *g_pHeightData;

/* This line is required for CImg to be able to read jpg/png format files. */
/* Please install ImageMagick and replace the path below to the correct path to convert.exe on your computer */
void initializeImageMagick()
{
	cimg::imagemagick_path("convert.exe", true);
}


/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
	int i;
	int j;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  CImg<unsigned char> in(640, 480, 1, 3, 0);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 in.data());
  }

  if (in.save_jpeg(filename))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

}

void myinit()
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
}

void display()
{
  int height = g_pHeightData->height();
  int width = g_pHeightData->width();
  /* draw 1x1 cube about origin */
  /* replace this code with your height field implementation */
  /* you may also want to precede it with your 
rotation/translation/scaling */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //creating values for lookat
  double ex = 0.0 + g_vLandRotate[0];
  double ey = 1.0 + g_vLandRotate[1];
  double ez = 3.0 + g_vLandRotate[2];
  double fx = 0.0;
  double fy = 0.0;
  double fz = 0.0;
  double ux = 0.0;
  double uy = -1.0;
  double uz = 0.0;
  //loading identity matrix and setting viewing perspective
  glLoadIdentity();
  gluPerspective(60.0, 1.0 * width / height, 0.01, 1000.0);   
  glMatrixMode(GL_MODELVIEW);
  //translating
  glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
  //altered rotation function to move camera because it is more intuitive
  //scaling
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
  //setting look at
  gluLookAt(ex, ey, ez, fx, fy, fz, ux, uy, uz);
  //determine render type
  switch (RenderState) {
	case WIREFRAME:
	  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	  break;
	case VERTICES:
	  glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	  break;
	case TRIANGLE:
	  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	  break;
  }
  //create vertices from image data
  for (int y = 0; y < (height)-1; y++) {  
	  /*Initialize triangle strip*/
	  glBegin(GL_TRIANGLE_STRIP);
	  for (int x =0; x < (width); x++) {
		  /*add a pixel and the one beneath it to the triange strip, using their height as the metric for the color produced
		  defaulting to a shade of blue*/
		  unsigned char* z1 = g_pHeightData->data(x, y);
		  unsigned char* z2 = g_pHeightData->data(x, y+1);
		  /*converting values to floats so they are normalized and usable*/
		  float fy = (float(y)-float(height/2)) / (height/2);
		  float fy2 = (float(y + 1)-float(height/2)) / (height/2);
		  float fx = (float(x)-float(width/2)) / (width/2);
		  float fz1 = (float(*z1) / 255)/3;
		  float fz2 = (float(*z2) / 255)/3;
		  float z3 = float(*z1) / 255;
		  float z4 = float(*z2) / 255;
		  float redbit = 0.0f;
		  float greenbit = 0.0f;
		  float bluebit = 0.0f;
		  /*creating vertices. color is based on colorstate, defaulting to blue*/
		  switch (color) {
			case RED:
				redbit = 1.0f;
				greenbit = z3;
				bluebit = z3;
				break;
			case BLUE:
				redbit = z3;
				greenbit = z3;
				bluebit = 1.0f;
				break;
			case PURPLEPINK:
				redbit = 1.0f;
				greenbit = z3;
				bluebit = 1.0f;
				break;
			case GREEN:
				redbit = z3;
				greenbit = 1.0f;
				bluebit = z3;
				break;
		  }
		  glColor3f(redbit, greenbit, bluebit);
		  glVertex3f(fx, fy, fz1);
		  //creating vertex 2
		  switch (color) {
		  case RED:
			  redbit = 1.0f;
			  greenbit = z4;
			  bluebit = z4;
			  break;
		  case BLUE:
			  redbit = z4;
			  greenbit = z4;
			  bluebit = 1.0f;
			  break;
		  case PURPLEPINK:
			  redbit = 1.0f;
			  greenbit = z4;
			  bluebit = 1.0f;
			  break;
		  case GREEN:
			  redbit = z4;
			  greenbit = 1.0f;
			  bluebit = z4;
			  break;
		  }
		  glColor3f(redbit, greenbit, bluebit);
		  glVertex3f(fx, fy2, fz2);
	  }
	  glEnd();
  }
  //swap buffers
  glutSwapBuffers();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
	case 1:
		RenderState = WIREFRAME;
		break;
	case 2: 
		RenderState = VERTICES;
		break;
	case 3: 
		RenderState = TRIANGLE;
		break;
	case 4:
		color = BLUE;
		break;
	case 5:
		color = RED;
		break;
	case 6:
		color = PURPLEPINK;
		break;
	case 7:
		color = GREEN;
		break;
  }
}

void doIdle()
{
  /* do some stuff... */
	//Couldn't get it to work. Data = /0 somewhere in the save_screenshot function. Using Fraps instead
  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
	/*changed rotation code to be like the translate so that mouse movements can move the camera
	instead of glrotate because I found it easier and more intuitive*/
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[0] * 0.02;
        g_vLandRotate[1] -= vMouseDelta[1] * 0.02;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1] * 0.02;
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.03;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.03;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.03;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

int main(int argc, char* argv[])
{
	// I've set the argv[1] to spiral.jpg.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your texture name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s heightfield.jpg\n", argv[0]);
		exit(1);
	}

	initializeImageMagick();
	g_pHeightData = new CImg<unsigned char>((char*)argv[1]);
	unsigned char* z1 = g_pHeightData->data(-128, -128);
	unsigned char* z2 = g_pHeightData->data(-127, -128);

	if (!g_pHeightData)
	{
	    printf ("error reading %s.\n", argv[1]);
	    exit(1);
	}
	
	glutInit(&argc,(char**)argv);
  
	/*
		create a window here..should be double buffered and use depth testing
  
	    the code past here will segfault if you don't have a window set up....
	    replace the exit once you add those calls.
	*/
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("test");
	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);
	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit",0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	glutAddMenuEntry("Wireframe", 1);
	glutAddMenuEntry("Vertices", 2);
	glutAddMenuEntry("Triangle", 3);
	glutAddMenuEntry("Blue", 4);
	glutAddMenuEntry("Red", 5);
	glutAddMenuEntry("Purple", 6);
	glutAddMenuEntry("Green", 7);
	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);

	/* do initialization */
	myinit();

	glutMainLoop();
	return 0;
}