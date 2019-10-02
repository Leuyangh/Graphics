/*
CSCI 480
Assignment 3 Raytracer

Name: <Your name here>
*/

#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <stdio.h>
#include <string>
#include <cmath>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename=0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode=MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

typedef struct _Triangle
{
  struct Vertex v[3];
  double getValue(int vertex, int axis) {
	  return v[vertex].position[axis];
  }
} Triangle;

typedef struct _Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  double position[3];
  double color[3];
} Light;

// point in 3d or vector from origin to its coords
// edit: point actually ended up acting as a vector a lot but im too lazy to rename cause it also got used as a color tuple a lot
struct point {
	double x, y, z;

	point() : x(0), y(0), z(0) {}
	point(double x2, double y2, double z2) : x(x2), y(y2), z(z2) {}

	
	point operator+ (const point& other) const { 
		return point(x + other.x, y + other.y, z + other.z);
	}
	point operator- (const point& other) const { 
		return point(x - other.x, y - other.y, z - other.z);
	}
	point operator* (double val) const {
		return point(x * val, y * val, z * val);
	}
	point& negate() {
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}
	double getLength() const {
		double temp = (x * x + y * y + z * z);
		return std::sqrt(temp);
	}
	point& normalize() {
		if (getLength() != 0) {
			x /= getLength();
			y /= getLength();
			z /= getLength();
		}
		return *this;
	}
	static point cross(const point& a, const point& b) {
		double x = a.y * b.z - a.z * b.y;
		double y = a.z * b.x - a.x * b.z;
		double z = a.x * b.y - a.y * b.x;
		return point(x, y, z);
	}
	double dot(const point& other) const {
		return (x*other.x + y * other.y + z * other.z);
	}
	point& clamp() {
		if (x < 0.0) {
			x = 0.0;
		}
		else if (x > 1.0) {
			x = 1.0;
		}
		if (y < 0.0) {
			y = 0.0;
		}
		else if (y > 1.0) {
			y = 1.0;
		}
		if (z < 0.0) {
			z = 0.0;
		}
		else if (z > 1.0) {
			z = 1.0;
		}
		return *this;
	}
};

//vector ended up being like a ray lol. got it backwards when i started. ray has an origin, vector is just magnitude and direction. dont know how I fucked that up
struct vector {
	point origin, direction;
	vector() : origin(point(0, 0, 0)), direction(point(1, 1, 1)) {}
	vector(point or , point dir) : origin(or), direction(dir) {}
	double length() {
		double temp = pow(origin.x - direction.x, 2) + pow(origin.y - direction.y, 2) + pow(origin.z - direction.z, 2);
		return std::sqrt(temp);
	}
	point toOriginVec() {
		double x = direction.x - origin.x;
		double y = direction.y - origin.y;
		double z = direction.z - origin.z;
		return point(x, y, z);
	}
	
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles=0;
int num_spheres=0;
int num_lights=0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

const double pi = 3.14159;

double toRad(double angle) {
	double tanAngle = (angle*pi) / 180;
	return tanAngle;
}
//point origin = point(0, 0, 0);
double closeEnoughToZero = 5 * pow(10, -10);
bool nearZero(double val) {
	if (val <= closeEnoughToZero && val >= -closeEnoughToZero) {
		return true;
	}
	return false;
}

void clamp(double& val) {
	if (val > 1.0) {
		val = 1.0;
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	return;
}

double min(double a, double b) {
	if (a < b) {
		return a;
	}
	else {
		return b;
	}
}
//lighting functions
point getReflection(point normal, point Ldirection) {
	double LN = Ldirection.dot(normal);
	clamp(LN);
	double xreflection = 2 * LN*normal.x - Ldirection.x; //2*l*n*n-l
	double yreflection = 2 * LN*normal.y - Ldirection.y;
	double zreflection = 2 * LN*normal.z - Ldirection.z;
	point reflection(xreflection, yreflection, zreflection);
	reflection.normalize();
	return reflection;
}
//R dot V
double getRV(point R, point intersection) {
	double RV;
	point V = intersection.negate();
	V.normalize();
	RV = R.dot(V);
	clamp(RV);
	return RV;
}
//test this vector against a triangle and store the intersection point in the point passed to it
bool testTriangle(point& intersectionPoint, Triangle triangle, vector ray) {
	//get vertices
	point A = point(triangle.getValue(0, 0), triangle.getValue(0, 1), triangle.getValue(0, 2));
	point B = point(triangle.getValue(1, 0), triangle.getValue(1, 1), triangle.getValue(1, 2));
	point C = point(triangle.getValue(2, 0), triangle.getValue(2, 1), triangle.getValue(2, 2));
	//make edges
	point edge1 = B - A;
	point edge2 = C - B;
	point edge3 = A - C;
	//get surface normal
	point normal = point::cross(edge1, C - A);
	normal.normalize();
	//check no-intersections
	double nrd = normal.dot(ray.direction); //normal-ray direction
	point distance = A - ray.origin;
	double D = distance.dot(normal);
	double t = -1;
	if (nrd != 0) {
		t = D / nrd;
	}
	if (nearZero(nrd) || t < 0) {
		return false; //ray and triangle parallel
	}
	//intersection with plane found because t >= 0 and triangle is not parallel to ray
	//update intersection point
	intersectionPoint = ray.origin + (ray.direction * t);
	//test intersection point within triangle
	point Atest = point::cross(edge1, intersectionPoint - A);
	point Btest = point::cross(edge2, intersectionPoint - B);
	point Ctest = point::cross(edge3, intersectionPoint - C);
	//dot products of normal and tests
	double NAtest = normal.dot(Atest);
	double NBtest = normal.dot(Btest);
	double NCtest = normal.dot(Ctest);
	//if any are negative, point is not within triangle
	if (NAtest < 0 || NBtest < 0 || NCtest < 0) {
		return false;
	}
	//within triangle and intersected plane
	return true;
}
//test if this vector intersects with the given sphere and store the point they intersect in the given variable
bool testSphere(point& intersectionPoint, Sphere sphere, vector ray) {
	point sphereCenter = point(sphere.position[0], sphere.position[1], sphere.position[2]); //xd
	point origintocenter = ray.origin - sphereCenter; // x0 - xc
	double A = ray.direction.x*ray.direction.x + ray.direction.y*ray.direction.y + ray.direction.z*ray.direction.z; //xd2 + yd2 + zd2
	double B = 2 * (ray.direction.x * (origintocenter.x) + ray.direction.y * (origintocenter.y) + ray.direction.z * (origintocenter.z)); //2(xd(x0-xc) + yd(y0-yc) + zd(z0-zc))
	double C = origintocenter.x * origintocenter.x + origintocenter.y * origintocenter.y + origintocenter.z * origintocenter.z - sphere.radius * sphere.radius; //(x0-xc)2 + (y0-yc)2 + (z0-zc)2 - r2
	double discriminant = B * B - (4 * A*C);
	if (discriminant < 0) {
		return false;
	}
	double t0 = (-B + std::sqrt(discriminant)) / 2;
	double t1 = (-B - std::sqrt(discriminant)) / 2;
	if (t0 < 0 || t1 < 0) {
		return false;
	}
	double t = min(t0, t1);
	intersectionPoint = ray.origin + (ray.direction * t);
	return true;
}
//area needed to get barycentric coords. easy way-projected to plane
double Area(point a, point b, point c) {
	double area;
	//determine which plane to project to
	point normal = point::cross(b - a, c - a);
	normal.normalize();
	if (normal.dot(point(0, 0, 1)) != 0) {
		area = 0.5*((b.x - a.x) * (c.y - a.y) - (c.x - a.x)*(b.y - a.y));
	}
	else {
		area = 0.5*((b.x - a.x) * (c.z - a.z) - (c.x - a.x)*(b.z - a.z));
	}
	return area;
}
//color of a point on triangle
point triangleColor(Triangle triangle, Light light, point intersection) {//"point" in this case is just a tuple that holds the r g b in its x y z. Returns point that represents color
	//For triangles, you should interpolate the x,y,z coordinates of the normals given at each vertex, and then normalize the length. 
	//Use barycentric coordinates for interpolation of triangles. You should interpolate not just the normals, but also diffuse, specular and shininess coefficients
	//get vertices
	point A = point(triangle.getValue(0, 0), triangle.getValue(0, 1), triangle.getValue(0, 2));
	point B = point(triangle.getValue(1, 0), triangle.getValue(1, 1), triangle.getValue(1, 2));
	point C = point(triangle.getValue(2, 0), triangle.getValue(2, 1), triangle.getValue(2, 2));
	//make edges
	point edge1 = B - A;
	point edge2 = C - B;
	point edge3 = A - C;
	//calculate apha beta gamma
	double alpha = Area(intersection, B, C) / Area(A, B, C);
	double beta = Area(A, intersection, C) / Area(A, B, C);
	double gamma = 1 - alpha - beta;
	//interpolate normals, specular, diffuse and shininess
	point specular = point(alpha * triangle.v[0].color_specular[0] + beta * triangle.v[1].color_specular[0] + gamma * triangle.v[2].color_specular[0], alpha * triangle.v[0].color_specular[1] + beta * triangle.v[1].color_specular[1] + gamma * triangle.v[2].color_specular[1], alpha * triangle.v[0].color_specular[2] + beta * triangle.v[1].color_specular[2] + gamma * triangle.v[2].color_specular[2]);
	point tNormal = point(alpha * triangle.v[0].normal[0] + beta * triangle.v[1].normal[0] + gamma * triangle.v[2].normal[0], alpha * triangle.v[0].normal[1] + beta * triangle.v[1].normal[1] + gamma * triangle.v[2].normal[1], alpha * triangle.v[0].normal[2] + beta * triangle.v[1].normal[2] + gamma * triangle.v[2].normal[2]);
	tNormal.normalize();
	point diffuse = point(alpha * triangle.v[0].color_diffuse[0] + beta * triangle.v[1].color_diffuse[0] + gamma * triangle.v[2].color_diffuse[0], alpha * triangle.v[0].color_diffuse[1] + beta * triangle.v[1].color_diffuse[1] + gamma * triangle.v[2].color_diffuse[1], alpha * triangle.v[0].color_diffuse[2] + beta * triangle.v[1].color_diffuse[2] + gamma * triangle.v[2].color_diffuse[2]);
	double shininess = alpha * triangle.v[0].shininess + beta * triangle.v[1].shininess + gamma * triangle.v[2].shininess;
	// need L*N and (2 * l (n - dir)) * dir, mag and reflect
	point lightorigin = point(light.position[0], light.position[1], light.position[2]);
	point Ldirection = lightorigin - intersection;
	Ldirection.normalize();
	double mag = Ldirection.dot(tNormal);
	point reflection = getReflection(tNormal, Ldirection);
	double reflectionMag = reflection.dot(intersection.negate().normalize());
	clamp(reflectionMag);
	double rgb[3];
	rgb[0] = light.color[0] * (diffuse.x * mag + (specular.x * std::pow(reflectionMag, shininess)));
	rgb[1] = light.color[1] * (diffuse.y * mag + (specular.y * std::pow(reflectionMag, shininess)));
	rgb[2] = light.color[2] * (diffuse.z * mag + (specular.z * std::pow(reflectionMag, shininess)));
	clamp(rgb[0]);
	clamp(rgb[1]);
	clamp(rgb[2]);
	point color(rgb[0], rgb[1], rgb[2]);
	return color;
}
//color of a point on sphere
point sphereColor(Sphere sphere, Light light, point intersection) { //"point" in this case is just a tuple that holds the r g b in its x y z. Returns point that represents color
	//normal = intersection-center / r
	point normal = intersection - point(sphere.position[0], sphere.position[1], sphere.position[2]);
	double rinverse = 1 / sphere.radius;
	normal = normal * rinverse;
	point Ldirection = point(light.position[0], light.position[1], light.position[2]) - intersection;
	Ldirection.normalize();
	point R = getReflection(normal, Ldirection);
	double RV = getRV(R, intersection);
	clamp(RV);
	double rgb[3];
	double LN = Ldirection.dot(normal);
	clamp(LN);
	for (int i = 0; i < 3; i++) {
		rgb[i] = light.color[i] * (sphere.color_diffuse[i] * LN + (sphere.color_specular[i] * std::pow(RV, sphere.shininess))); //diffuse * L*N + specular * (r*v) ^ sh
		clamp(rgb[i]);
	}
	point color(rgb[0], rgb[1], rgb[2]);
	return color;
}
//create ray at x,y
vector createRay(int x, int y) {
	double aspectRatio = double(WIDTH) / double(HEIGHT);
	double fovAngle = std::tan(toRad(fov / 2.0));
	//convert x to screen coordinates
	double screenX = (2 * ((x + 0.5) / double(WIDTH)) - 1);
	double screenY = (2 * ((y + 0.5) / double(HEIGHT)) - 1);
	point rayDir(screenX * aspectRatio * fovAngle, screenY * fovAngle, -1);
	rayDir.normalize();
	vector ray(point(0, 0, 0), rayDir);
	return ray;
}
//trace ray. check closest collision and color accordingly
point raytrace(vector& ray) { //point is once again just a tuple of doubles representing color values rgb
	point color = point(1, 1, 1); //default to white
	double closestTriangle = -1.0 * pow(2, 32); //some arbitrarily large distance away
	double closestSphere = -1.0 * pow(2, 32);
	//store information about closest intersections
	int closestTIndex = -1;
	int closestSIndex = -1;
	point closestTIntersection;
	point closestSIntersection;
	//go through all triangles and test for intersections
	for (int i = 0; i < num_triangles; i++) {
		point closest = point(0, 0, closestTriangle);
		bool intersects = testTriangle(closest, triangles[i], ray);
		if (intersects && closest.z > closestTriangle) {
			closestTriangle = closest.z;
			closestTIndex = i;
			closestTIntersection = closest;
		}
	}
	//test for closest sphere
	for (int i = 0; i < num_spheres; i++) {
		point closest = point(0, 0, closestSphere);
		bool intersects = testSphere(closest, spheres[i], ray);
		if (intersects && closest.z > closestSphere) {
			closestSphere = closest.z;
			closestSIndex = i;
			closestSIntersection = closest;
		}
	}
	//color sphere or triangle, whichever is closest
	if (closestTriangle > closestSphere && closestTIndex != -1) { //triangle is closer
		color = point(0, 0, 0);
		//check which lights affect the triangle
		for (int i = 0; i < num_lights; i++) {
			//create shadow ray to light
			point lightorigin = point(lights[i].position[0], lights[i].position[1], lights[i].position[2]);
			point shadoworigin = closestTIntersection;
			point shadowdirection = lightorigin - shadoworigin;
			shadowdirection.normalize();
			vector shadowray = vector(shadoworigin, shadowdirection);
			bool hitslight = true;
			//test if it hits a sphere before the light
			for(int j = 0; j < num_spheres; j++){
				point sIntersect;
				bool intersectsSphere = testSphere(sIntersect, spheres[j], shadowray);
				if (intersectsSphere) {
					double sphereIntersectionDistance = (sIntersect - closestTIntersection).getLength();
					double lightIntersectionDistance = (lightorigin - closestTIntersection).getLength();
					if (lightIntersectionDistance > sphereIntersectionDistance){
						hitslight = false;
						break;
					}
				}
			}
			//test if it hits a triangle that isnt this one
			for (int j = 0; j < num_triangles; j++) {
				if (!hitslight) {
					j = num_triangles;
				}
				point tIntersect;
				bool intersectsTriangle = testTriangle(tIntersect, triangles[j], shadowray);
				if (intersectsTriangle && j != closestTIndex) {
					double sphereIntersectionDistance = (tIntersect - closestTIntersection).getLength();
					double lightIntersectionDistance = (lightorigin - closestTIntersection).getLength();
					if (lightIntersectionDistance > sphereIntersectionDistance) {
						hitslight = false;
						break;
					}
				}
			}
			if (hitslight) {
				color = color + triangleColor(triangles[closestTIndex], lights[i], closestTIntersection).clamp();
			}
		}
	}
	//sphere was closer
	else if (closestSphere > closestTriangle && closestSIndex != -1) { //sphere is closer
		color = point(0, 0, 0);
		//check which lights affect the sphere
		for (int i = 0; i < num_lights; i++) {
			//create shadow ray to light
			point lightorigin = point(lights[i].position[0], lights[i].position[1], lights[i].position[2]);
			point shadoworigin = closestSIntersection;
			point shadowdirection = lightorigin - shadoworigin;
			shadowdirection.normalize();
			vector shadowray = vector(shadoworigin, shadowdirection);
			bool hitslight = true;
			//test if it hits a sphere before the light
			for (int j = 0; j < num_spheres; j++) {
				point sIntersect;
				bool intersectsSphere = testSphere(sIntersect, spheres[j], shadowray);
				if (intersectsSphere && j != closestSIndex) {
					double sphereIntersectionDistance = (sIntersect - closestSIntersection).getLength();
					double lightIntersectionDistance = (lightorigin - closestSIntersection).getLength();
					if (sphereIntersectionDistance < lightIntersectionDistance) {
						hitslight = false;
						break;
					}
				}
			}
			//test if it hits a triangle
			for (int j = 0; j < num_triangles; j++) {
				if (!hitslight) {
					j = num_triangles;
				}
				point tIntersect;
				bool intersectsTriangle = testTriangle(tIntersect, triangles[j], shadowray);
				if (intersectsTriangle) {
					double triangleIntersectionDistance = (tIntersect - closestSIntersection).getLength();
					double lightIntersectionDistance = (lightorigin - closestSIntersection).getLength();
					if (triangleIntersectionDistance < lightIntersectionDistance) {
						hitslight = false;
						break;
					}
				}
			}
			if (hitslight) {
				color = color + sphereColor(spheres[closestSIndex], lights[i], closestSIntersection).clamp();
			}
		}
	}
	double rgb[3];
	//add ambient light to create final color
	rgb[0] = color.x + ambient_light[0];
	rgb[1] = color.y + ambient_light[1];
	rgb[2] = color.z + ambient_light[2];
	clamp(rgb[0]);
	clamp(rgb[1]);
	clamp(rgb[2]);
	point finalcolor(rgb[0], rgb[1], rgb[2]);
	return finalcolor;
}
//MODIFY THIS FUNCTION
void draw_scene()
{
  unsigned int x,y;
  //simple output
  for(x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(y=0;y < HEIGHT;y++)
    {
		vector ray = createRay(x, y);
		point color = raytrace(ray);
		plot_pixel(x, y, color.x * 255.0f, color.y * 255.0f, color.z * 255.0f);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}

void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}

/* Write a jpg image from buffer*/
void save_jpg()
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferBGR = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	// unsigned char buffer[HEIGHT][WIDTH][3];
	for (int r = 0; r < HEIGHT; r++) {
		for (int c = 0; c < WIDTH; c++) {
			for (int chan = 0; chan < 3; chan++) {
				unsigned char red = buffer[r][c][0];
				unsigned char green = buffer[r][c][1];
				unsigned char blue = buffer[r][c][2];
				bufferBGR.at<cv::Vec3b>(r,c) = cv::Vec3b(blue, green, red);
			}
		}
	}
	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

void parse_check(char *expected,char *found)
{
  if(stricmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}

void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i",&number_of_objects);

  printf("number of objects: %i\n",number_of_objects);
  char str[200];

  parse_doubles(file,"amb:",ambient_light);

  for(i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
      if(stricmp(type,"triangle")==0)
	{

	  printf("found triangle\n");
	  int j;

	  for(j=0;j < 3;j++)
	    {
	      parse_doubles(file,"pos:",t.v[j].position);
	      parse_doubles(file,"nor:",t.v[j].normal);
	      parse_doubles(file,"dif:",t.v[j].color_diffuse);
	      parse_doubles(file,"spe:",t.v[j].color_specular);
	      parse_shi(file,&t.v[j].shininess);
	    }

	  if(num_triangles == MAX_TRIANGLES)
	    {
	      printf("too many triangles, you should increase MAX_TRIANGLES!\n");
	      exit(0);
	    }
	  triangles[num_triangles++] = t;
	}
      else if(stricmp(type,"sphere")==0)
	{
	  printf("found sphere\n");

	  parse_doubles(file,"pos:",s.position);
	  parse_rad(file,&s.radius);
	  parse_doubles(file,"dif:",s.color_diffuse);
	  parse_doubles(file,"spe:",s.color_specular);
	  parse_shi(file,&s.shininess);

	  if(num_spheres == MAX_SPHERES)
	    {
	      printf("too many spheres, you should increase MAX_SPHERES!\n");
	      exit(0);
	    }
	  spheres[num_spheres++] = s;
	}
      else if(stricmp(type,"light")==0)
	{
	  printf("found light\n");
	  parse_doubles(file,"pos:",l.position);
	  parse_doubles(file,"col:",l.color);

	  if(num_lights == MAX_LIGHTS)
	    {
	      printf("too many lights, you should increase MAX_LIGHTS!\n");
	      exit(0);
	    }
	  lights[num_lights++] = l;
	}
      else
	{
	  printf("unknown type in scene description:\n%s\n",type);
	  exit(0);
	}
    }
  return 0;
}

void display()
{

}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
      draw_scene();
      if(mode == MODE_JPEG)
	save_jpg();
    }
  once=1;
}

int main (int argc, char ** argv)
{
  if (argc<2 || argc > 3)
  {  
    printf ("usage: %s <scenefile> [jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
    {
      mode = MODE_JPEG;
      filename = argv[2];
    }
  else if(argc == 2)
    mode = MODE_DISPLAY;
  
  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
