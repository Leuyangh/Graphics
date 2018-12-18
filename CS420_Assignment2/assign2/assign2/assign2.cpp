// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 480 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	C++ starter code
*/

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include <cmath>
#include <vector>
/* Object where you can load an image */
cv::Mat3b imageBGR;
GLuint texture[2];
/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;
float g_vLandRotate[3] = { 0.0, 0.0, 0.0 };
float g_vLandTranslate[3] = { 0.0, 0.0, 0.0 };
float g_vLandScale[3] = { 1.0, 1.0, 1.0 };
int g_iMenuId;
int screenshotCount = 1000; // change to 0 in order to save screenshots
int g_vMousePos[2] = { 0, 0 };
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;
int currentPoint = 0;
int defaultspeed = 1;
int speed = 1; //how fast camera moves along splines
int crossFreq = 10; //how often to draw cross beam
int defaultCrossFreq = 10;
bool portals = false; //just for fun
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;
std::vector<point> allPoints;
std::vector<point> pointsForCrossSection;
void setCamera(int startingPoint);
//pre-declared functions
point getCameraNormal(point cameraTangent);
point getCameraTangent(point a, point b, point c, point d);
point getCameraBinormal(point normal, point cameraTangent);

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);
	printf("%d\n", g_iNumOfSplines);
	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

/* Write a screenshot to the specified filename */
void saveScreenshot(const char *filename)
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferRGB = cv::Mat::zeros(480, 640, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	//use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (bufferRGB.step & 3) ? 1 : 4);
	//set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, bufferRGB.step / bufferRGB.elemSize());
	glReadPixels(0, 0, bufferRGB.cols, bufferRGB.rows, GL_RGB, GL_UNSIGNED_BYTE, bufferRGB.data);
	//flip to account for GL 0,0 at lower left
	cv::flip(bufferRGB, bufferRGB, 0);
	//convert RGB to BGR
	cv::Mat3b bufferBGR(bufferRGB.rows, bufferRGB.cols, CV_8UC3);
	cv::Mat3b out[] = { bufferBGR };
	// rgb[0] -> bgr[2], rgba[1] -> bgr[1], rgb[2] -> bgr[0]
	int from_to[] = { 0,2, 1,1, 2,0 };
	mixChannels(&bufferRGB, 1, out, 1, from_to, 3);

	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

/* Function to get a pixel value. Use like PIC_PIXEL macro. 
Note: OpenCV images are in channel order BGR. 
This means that:
chan = 0 returns BLUE, 
chan = 1 returns GREEN, 
chan = 2 returns RED. */
unsigned char getPixelValue(cv::Mat3b& image, int x, int y, int chan)
{
	return image.at<cv::Vec3b>(y, x)[chan];
}

/* Function that does nothing but demonstrates looping through image coordinates.*/
void loopImage(cv::Mat3b& image)
{
	for (int r = 0; r < image.rows; r++) { // y-coordinate
		for (int c = 0; c < image.cols; c++) { // x-coordinate
			for (int channel = 0; channel < 3; channel++) {
				// DO SOMETHING... example usage
				unsigned char blue = getPixelValue(image, c, r, 0);
				// unsigned char green = getPixelValue(image, c, r, 1); 
				unsigned char red = getPixelValue(image, c, r, 2); 
				image.at<cv::Vec3b>(r, c)[0] = red;
				image.at<cv::Vec3b>(r, c)[2] = blue;
			}
		}
	}
}

/* Read an image into memory. 
Set argument displayOn to true to make sure images are loaded correctly.
One image loaded, set to false so it doesn't interfere with OpenGL window.*/
int readImage(char *filename, cv::Mat3b& image, bool displayOn)
{
	std::cout << "reading image: " << filename << std::endl;
	image = cv::imread(filename);
	if (!image.data) // Check for invalid input                    
	{
		std::cout << "Could not open or find the image." << std::endl;
		return 1;
	}

	if (displayOn)
	{
		cv::imshow("TestWindow", image);
		cv::waitKey(0); // Press any key to enter. 
	}
	return 0;
}

void texload(int i, char *filename)
{
	readImage(filename, imageBGR, false);
	loopImage(imageBGR);
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGB,
		imageBGR.cols,
		imageBGR.rows,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		imageBGR.data);
}

void myinit()
{
	//set up texture
	glGenTextures(4, texture);
	texload(0, "ground3.jpg");
	texload(1, "sky2.jpg");
	texload(2, "track_cross.jpg");
	texload(3, "portal.jpg");
	/* setup gl view here */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
}

void display()
{	
	//set window size
	int height = 480;
	int width = 640;
	//clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//load texture ground
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(50.0, 50.0, 50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-50.0, 50.0, 50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-50.0, 50.0, -50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(50.0, 50.0, -50.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	//load sky texture cube wall -x side
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-50.0, 50.0, 50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-50.0, -50.0, 50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-50.0, -50.0, -50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-50.0, 50.0, -50.0);
	glEnd();
	//load sky texture cube wall +x side
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(50.0, 50.0, 50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(50.0, -50.0, 50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(50.0, -50.0, -50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(50.0, 50.0, -50.0);
	glEnd();
	//load sky texture cube wall -z side
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(50.0, 50.0, -50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(50.0, -50.0, -50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-50.0, -50.0, -50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-50.0, 50.0, -50.0);
	glEnd();
	//load sky texture cube wall +z side
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(50.0, 50.0, 50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(50.0, -50.0, 50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-50.0, -50.0, 50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-50.0, 50.0, 50.0);
	glEnd();
	//load sky texture cube wall top side
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(50.0, -50.0, 50.0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-50.0, -50.0, 50.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-50.0, -50.0, -50.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(50.0, -50.0, -50.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	//end ground
	//calculate normal
	point tangent = getCameraTangent(allPoints[currentPoint], allPoints[currentPoint + 1], allPoints[currentPoint + 2], allPoints[currentPoint + 3]);
	//scale normal down
	point normal = getCameraNormal(tangent);
	normal.x = 0.1 * normal.x;
	normal.y = 0.1 * normal.y;
	normal.z = 0.1 * normal.z;
	glBegin(GL_LINE_STRIP); //Begin drawing line segment 2 from spline points (add normal)
	glLineWidth(1.0f); //useless? can't tell yet
	for (int i = 0; i < allPoints.size() - 1; i++) {//go through all points and draw them as part of the line strip
		point current = allPoints[i];
		point next = allPoints[i + 1];
		if (abs(next.x - current.x) < .1 && abs(next.y - current.y) < .1 && abs(next.z - current.z) < .1) { //check next point is part of current spline
			glVertex3d(current.x + normal.x, current.y, current.z + normal.z);
		}
		else { //end current spline if it is not and start a new one
			glEnd();
			//draw portal
			if (portals) {
				point portalTan = getCameraTangent(allPoints[i - 3], allPoints[i - 2], allPoints[i - 1], allPoints[i]);
				point portalNorm = getCameraNormal(portalTan);
				portalTan = getCameraBinormal(portalNorm, portalTan);
				portalTan.x *= 0.3;
				portalTan.y *= 0.3;
				portalTan.z *= 0.3;
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, texture[3]);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glBegin(GL_POLYGON);
				glTexCoord2f(1.0, 0.0);
				glVertex3d(current.x + normal.x, current.y, current.z + normal.z); //points on track
				glTexCoord2f(0.0, 0.0);
				glVertex3d(current.x - normal.x, current.y, current.z - normal.z);
				glTexCoord2f(0.0, 1.0);
				glVertex3d(current.x - normal.x + portalTan.x, current.y + portalTan.y, current.z - normal.z + portalTan.z); //portalTan is actually binormal at this point, points "above" track
				glTexCoord2f(1.0, 1.0);
				glVertex3d(current.x + normal.x + portalTan.x, current.y + portalTan.y, current.z + normal.z + portalTan.z);
				glEnd();
				glDisable(GL_TEXTURE_2D);
			}
			glBegin(GL_LINE_STRIP);
		}
	}
	glEnd(); // done
	glBegin(GL_LINE_STRIP); //Begin drawing line segment 3 from spline points (sub normal)
	glLineWidth(0.5f); //useless? can't tell yet
	for (int i = 0; i < allPoints.size() - 1; i++) {//go through all points and draw them as part of the line strip
		point current = allPoints[i];
		point next = allPoints[i + 1];
		if (abs(next.x - current.x) < .1 && abs(next.y - current.y) < .1 && abs(next.z - current.z) < .1) {//check next point is part of current spline
			glVertex3d(current.x - normal.x, current.y, current.z - normal.z);
		}
		else {//end current spline if it is not and start a new one
			glEnd();
			glBegin(GL_LINE_STRIP);
		}
	}
	glEnd();
	//begin drawing cross-section
	for (int i = 0; i < allPoints.size() - 4; i += crossFreq) {
		point a = allPoints[i]; //points on parallel tracks
		point b = allPoints[i];
		double s = 0.01;
		a.x += normal.x;
		a.z += normal.z;
		b.x -= normal.x;
		b.z -= normal.z;
		point tan = getCameraTangent(allPoints[i], allPoints[i + 1], allPoints[i + 2], allPoints[i + 3]);
		point norm = getCameraNormal(tan);
		point binorm = getCameraBinormal(norm, tan);
		point v1 = a; //set points 1-8
		v1.x += s * (tan.x - binorm.x);
		v1.y += s * (tan.y - binorm.y);
		v1.z += s * (tan.z - binorm.z);
		point v2 = a;
		v2.x += s * (tan.x + binorm.x);
		v2.y += s * (tan.y + binorm.y);
		v2.z += s * (tan.z + binorm.z);
		point v3 = a;
		v3.x += s * (-tan.x + binorm.x);
		v3.y += s * (-tan.y + binorm.y);
		v3.z += s * (-tan.z + binorm.z);
		point v4 = a;
		v4.x += s * (-tan.x - binorm.x);
		v4.y += s * (-tan.y - binorm.y);
		v4.z += s * (-tan.z - binorm.z);
		point v5 = b;
		v5.x += s * (tan.x - binorm.x);
		v5.y += s * (tan.y - binorm.y);
		v5.z += s * (tan.z - binorm.z);
		point v6 = b;
		v6.x += s * (tan.x + binorm.x);
		v6.y += s * (tan.y + binorm.y);
		v6.z += s * (tan.z + binorm.z);
		point v7 = b;
		v7.x += s * (-tan.x + binorm.x);
		v7.y += s * (-tan.y + binorm.y);
		v7.z += s * (-tan.z + binorm.z);
		point v8 = b;
		v8.x += s * (-tan.x - binorm.x);
		v8.y += s * (-tan.y - binorm.y);
		v8.z += s * (-tan.z - binorm.z);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBegin(GL_POLYGON); //drawing 6 surfaces of box
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v1.x, v1.y, v1.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v2.x, v2.y, v2.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v3.x, v3.y, v3.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v4.x, v4.y, v4.z);
		glEnd();
		glBegin(GL_POLYGON);
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v1.x, v1.y, v1.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v2.x, v2.y, v2.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v6.x, v6.y, v6.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v5.x, v5.y, v5.z);
		glEnd();
		glBegin(GL_POLYGON);
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v1.x, v1.y, v1.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v4.x, v4.y, v4.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v8.x, v8.y, v8.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v5.x, v5.y, v5.z);
		glEnd();
		glBegin(GL_POLYGON);
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v4.x, v4.y, v4.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v3.x, v3.y, v3.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v7.x, v7.y, v7.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v8.x, v8.y, v8.z);
		glEnd();
		glBegin(GL_POLYGON);
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v2.x, v2.y, v2.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v3.x, v3.y, v3.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v7.x, v7.y, v7.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v6.x, v6.y, v6.z);
		glEnd();
		glBegin(GL_POLYGON);
		glTexCoord2f(1.0, 0.0);
		glVertex3d(v5.x, v5.y, v5.z);
		glTexCoord2f(0.0, 0.0);
		glVertex3d(v6.x, v6.y, v6.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3d(v7.x, v7.y, v7.z);
		glTexCoord2f(1.0, 1.0);
		glVertex3d(v8.x, v8.y, v8.z);
		glEnd();
		
		glDisable(GL_TEXTURE_2D);
		/*if (i + crossFreq > allPoints.size() - 4) {
			i = allPoints.size() - 4 - crossFreq;
		}*/
	}


	//loading identity matrix and setting viewing perspective
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluPerspective(60.0, 1.0 * width / height, 0.01, 1000.0);
	
	
	
	//altered rotation function to move camera because it is more intuitive
	//scaling
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
	glRotatef(g_vLandRotate[0], 1.0f, 0.0f, 0.0f);
	glRotatef(g_vLandRotate[1], 0.0f, 1.0f, 0.0f);
	glRotatef(g_vLandRotate[2], 0.0f, 0.0f, 1.0f);
	//translating
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	//gluLookAt(ex, ey, ez, fx, fy, fz, ux, uy, uz); changed to use compute camera function\
	//setting look at
	setCamera(currentPoint);
	if (currentPoint + speed <= allPoints.size() - 4) {
		currentPoint += speed;
	}
	//determine render type
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
		currentPoint = 0;
		speed = defaultspeed;
		crossFreq = defaultCrossFreq;
		break;
	case 2:
		speed++;
		break;
	case 3:
		speed += 5;
		break;
	case 4:
		crossFreq += 10;
		break;
	case 5:
		crossFreq += 100;
		break;
	case 6:
		portals = !portals;
		break;
	case 7:
		g_vLandRotate[0] = 0;
		g_vLandRotate[1] = 0;
		g_vLandRotate[2] = 0;
		break;
	case 8:
		speed = defaultspeed;
		break;
	case 9:
		speed = 0;
		break;
	}
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y)
{
	int vMouseDelta[2] = { x - g_vMousePos[0], y - g_vMousePos[1] };

	switch (g_ControlState)
	{
	case TRANSLATE:
		if (g_iLeftMouseButton)
		{
			g_vLandTranslate[0] += vMouseDelta[0] * 0.01;
			g_vLandTranslate[1] -= vMouseDelta[1] * 0.01;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandTranslate[2] += vMouseDelta[1] * 0.01;
		}
		break;
	case ROTATE:
		if (g_iLeftMouseButton)
		{
			g_vLandRotate[0] += vMouseDelta[1];
			g_vLandRotate[1] += vMouseDelta[0];
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandRotate[2] += vMouseDelta[1];
		}
		break;
	case SCALE:
		if (g_iLeftMouseButton)
		{
			g_vLandScale[0] *= 1.0 + vMouseDelta[0] * 0.03;
			g_vLandScale[1] *= 1.0 - vMouseDelta[1] * 0.03;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandScale[2] *= 1.0 - vMouseDelta[1] * 0.03;
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
		g_iLeftMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		g_iMiddleMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		g_iRightMouseButton = (state == GLUT_DOWN);
		break;
	}

	switch (glutGetModifiers())
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

void doIdle()
{
	/* do some stuff... */
	//save screen shot to anim folder for first 1000 frames
	if (screenshotCount < 1000) {
		std::stringstream filename;
		filename << "anim/" << screenshotCount << ".jpg";
		std::string temp = filename.str();
		const char* name = temp.c_str();
		saveScreenshot(name);
		screenshotCount++;
	}
	
	/* make the screen update */
	glutPostRedisplay();
}
/* OpenCV help:
Access number of rows of image (height): image.rows; 
Access number of columns of image (width): image.cols;
Pixel 0,0 is the upper left corner. Byte order for 3-channel images is BGR. 
*/

point getCameraTangent(point a, point b, point c, point d) { //calculate tangent pretty much
	double u = 0.005;
	point tangent;
	tangent.x = 0.5 * ((-a.x + c.x) + (2 * a.x - 5 * b.x + 4 * c.x - d.x) * (2 * u) + (-a.x + 3 * b.x - 3 * c.x + d.x) * (3 * u * u)); //calculate point xyz from formula of derivative
	tangent.y = 0.5 * ((-a.y + c.y) + (2 * a.y - 5 * b.y + 4 * c.y - d.y) * (2 * u) + (-a.y + 3 * b.y - 3 * c.y + d.y) * (3 * u * u)); //tangent (orientation): t(u) =unit(p'(u)) = unit([3u^2 2u 1 0] M C).
	tangent.z = 0.5 * ((-a.z + c.z) + (2 * a.z - 5 * b.z + 4 * c.z - d.z) * (2 * u) + (-a.z + 3 * b.z - 3 * c.z + d.x) * (3 * u * u));
	double magnitude = sqrt(pow(tangent.x, 2) + pow(tangent.y, 2) + pow(tangent.z, 2)); //get magnitude for normalizing
	tangent.x = tangent.x / magnitude; // normalize
	tangent.y = tangent.y / magnitude;
	tangent.z = tangent.z / magnitude;
	return tangent;
}

point getCameraNormal(point cameraTangent) {
	//Pick an arbitrary unit(V). N0 = unit(T0 x V) and B0 = unit(T0 x N0). This guarantees T0, N0, B0 orthonormal to each other.
	point normal;
	//arbitrary unit(v) = world up vector (0, 1, 0)
	normal.x = (-(cameraTangent.z * 1));
	normal.y = 0;
	normal.z = (cameraTangent.x * 1);
	return normal;
}

point getCameraBinormal(point normal, point cameraTangent) {
	point binormal;
	binormal.x = (normal.y * cameraTangent.z - normal.z * cameraTangent.y);
	binormal.y = -(normal.x * cameraTangent.z - normal.z * cameraTangent.x);
	binormal.z = (normal.x * cameraTangent.y - normal.y * cameraTangent.z);
	return binormal;
}

void setCamera(int startPoint) {
	point a = allPoints[startPoint];
	point b = allPoints[startPoint + 1];
	point c = allPoints[startPoint + 2];
	point d = allPoints[startPoint + 3];
	point tangent = getCameraTangent(a, b, c, d);
	point normal = getCameraNormal(tangent);
	point binormal = getCameraBinormal(normal, tangent);
	gluLookAt(a.x, a.y + 0.1, a.z, a.x + tangent.x, a.y + tangent.y + 0.1, a.z + tangent.z, binormal.x, binormal.y, binormal.z);
	return;
}

void createSplines() {
	for (int i = 0; i < g_iNumOfSplines; i++) { //create proper # of splines
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			point a = g_Splines[i].points[j];
			point b = g_Splines[i].points[j + 1];
			point c = g_Splines[i].points[j + 2];
			point d = g_Splines[i].points[j + 3];
			/*q(t) = 0.5 *((2 * P1) + -P0 + P2) * t + (2*P0 - 5*P1 + 4*P2 - P3) * t2 + (-P0 + 3*P1- 3*P2 + P3) * t3) (From MVPS.org)*/
			for (double u = 0; u < 1.0; u += 0.005) { //create points at t(0) through t(1)
			//calculate x,y,z components
				point current;
				current.x = 0.5 * ((2 * b.x) + (-a.x + c.x) * u + (2 * a.x - 5 * b.x + 4 * c.x - d.x) * (pow(u,2)) + (-a.x + 3 * b.x - 3 * c.x + d.x) * (pow(u,3)));
				current.y = 0.5 * ((2 * b.y) + (-a.y + c.y) * u + (2 * a.y - 5 * b.y + 4 * c.y - d.y) * (pow(u, 2)) + (-a.y + 3 * b.y - 3 * c.y + d.y) * (pow(u, 3)));
				current.z = 0.5 * ((2 * b.z) + (-a.z + c.z) * u + (2 * a.z - 5 * b.z + 4 * c.z - d.z) * (pow(u, 2)) + (-a.z + 3 * b.z - 3 * c.z + d.z) * (pow(u, 3)));
				//add point to vector of all points
				allPoints.push_back(current);
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to track.txt.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your track file name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}
	
	loadSplines(argv[1]);
	createSplines();
	// If you need to load textures use below instead of pic library:
	// readImage("spiral.jpg", imageBGR, true);

	// Demonstrates to loop across image and access pixel values:
	// Note this function doesn't do anything, but you may find it useful:
	// loopImage(imageBGR);

	// Rebuilt save screenshot function, but will seg-fault unless you have
	// OpenGL framebuffers initialized. You can use this new function as before:
	// saveScreenshot("test_screenshot.jpg");
	glutInit(&argc, (char**)argv);

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
	glutAddMenuEntry("Quit", 0);
	glutAddMenuEntry("Reset", 1);
	glutAddMenuEntry("Speed up", 2);
	glutAddMenuEntry("Speed up a lot", 3);
	glutAddMenuEntry("Decrease crossbar frequency (Risky)", 4);
	glutAddMenuEntry("Decrease crossbar frequency a lot (Risky)", 5);
	glutAddMenuEntry("Toggle Portals", 6);
	glutAddMenuEntry("Reset camera", 7);
	glutAddMenuEntry("Reset speed", 8);
	glutAddMenuEntry("Stop coaster", 9);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
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
