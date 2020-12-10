#include "Render.h"

#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"

#include <algorithm>
#include <cmath>
#include <chrono>

bool textureMode = true;
bool lightMode = true;

using namespace std;

typedef long double ld;


struct point
{
	ld x, y, z;

	point() {}

	point(ld x, ld y, ld z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void AddNormal()
	{
		ld len = get_rad_len();
		glNormal3d(x / len, y / len, z / len);

	}

	void AddThis()
	{
		glVertex3d(x, y, z);
	}

	point turn(const point& p) const //xOy only
	{
		point res;
		res.z = z;
		if (p.x == 0 && p.y == 0)
		{
			res.x = x;
			res.y = y;
			return res;
		}
		res.x = x * p.x - y * p.y;
		res.y = x * p.y + y * p.x;
		return res;
	}

	void normalize()
	{
		ld len = get_rad_len();
		x /= len, y /= len, z /= len;
	}

	point operator+(const point& p) const
	{
		return point(x + p.x, y + p.y, z + p.z);
	}

	point operator-(const point& p) const
	{
		return point(x - p.x, y - p.y, z - p.z);
	}

	point operator*(ld a) const
	{
		return point(x * a, y * a, z * a);
	}

	point operator/(ld a) const
	{
		return point(x / a, y / a, z / a);
	}

	ld scalar_product(const point& p)const
	{
		return x * p.x + y * p.y + z * p.z;
	}

	point operator*(const point& p) const
	{
		point res;
		res.x = z * p.y - y * p.z;
		res.y = x * p.z - z * p.x;
		res.z = y * p.x - x * p.y;
		return res * -1;
	}

	ld get_range(const point& p) const
	{
		return sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y) + (z - p.z) * (z - p.z));
	}

	ld get_rad_len()
	{
		return get_range({ 0, 0, 0 });
	}

	void addXYCoord(ld scale)
	{
		glTexCoord2d(x / 2 / scale + 0.5, y / 2 / scale + 0.5);
	}

	~point()
	{

	}
};

void randomColor()
{
	glColor3d((ld)((rand() % 999) + 1) / 1000, (ld)((rand() % 999) + 1) / 1000, (ld)((rand() % 999) + 1) / 1000);
}

void triangle(point a, point b, point c, point shift = { 0, 0, 0 })
{
	((b - a) * (c - a)).AddNormal();
	(a + shift).AddThis();
	(b + shift).AddThis();
	(c + shift).AddThis();
}

void texture_triangle(point a, point b, point c, ld scale = 1, point shift = { 0, 0, 0 })
{
	((b - a) * (c - a)).AddNormal();
	(a + shift).addXYCoord(scale);
	(a + shift).AddThis();
	(b + shift).addXYCoord(scale); 
	(b + shift).AddThis();
	(c + shift).addXYCoord(scale); 
	(c + shift).AddThis();
}

void turned_quad(point a, point b, point height, point turn_point, int num, point shift = { 0, 0, 0 })
{
	point move = (b - a) / num, prev = a, next;
	for (int i = 0; i < num; ++i)
	{
		next = prev + move;
		triangle(prev, next.turn(turn_point) + height / num, next, shift);
		triangle(prev, prev.turn(turn_point) + height / num, next.turn(turn_point) + height / num, shift);
		prev = next;
	}
	
	//triangle(a, b.turn(turn_point) + height / num, b, shift);
	//triangle(a, a.turn(turn_point) + height / num, b.turn(turn_point) + height / num, shift);
}


//---------------------------------------defaults-----------------------------------------------------------------


//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}

void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{
	
}



GLuint texId;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	

	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE *texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char *texCharArray;
	int texW, texH;
	OpenGL::LoadBMP("texture.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);

	
	
	//генерируем ИД для текстуры
	glGenTextures(1, &texId);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}


void prizm()
{

	point a(-4, -6, 0), b(-5, 2, 0), c(1, 7, 0), d(-1, 3, 0), e(3, 1, 0), f(6, 3, 0), g(1, -6, 0), k(1, -1, 0);
	point a1 = a, b1 = b, c1 = c, d1 = d, e1 = e, f1 = f, g1 = g, k1 = k;
	point O1 = (f + g) / 2, O2(-9, -2.5625, 0), height(0, 0, 10);
	ld total_turn = -PI / 2;
	vector <point> ABArc = { a };
	ld alpha = acos((a - O2).scalar_product((b - O2)) / (a - O2).get_rad_len() / (b - O2).get_rad_len());// angle length of AB arc(O2 circle)
	point turn_point(cos(alpha / 99), sin(alpha / 99), 0);
	for (int i = 0; i < 99; ++i)
	{
		ABArc.push_back((ABArc[i] - O2).turn(turn_point) + O2);
	}

	vector <point> FGArc = { f };
	ld beta = acos((f - O1).scalar_product((g - O1)) / (f - O1).get_rad_len() / (g - O1).get_rad_len());// angle length of FG arc(O1 circle)
	turn_point = point(cos(beta / 99), -sin(beta / 99), 0);
	for (int i = 0; i < 99; ++i)
	{
		FGArc.push_back((FGArc[i] - O1).turn(turn_point) + O1);
	}

	int midpoint = 70;

	//------------ drawing ----------------
	glBegin(GL_TRIANGLES);
	glColor3d(0.8, 0.2, 0);
	//bottom
	for (int i = 0; i < midpoint; ++i)
	{
		triangle(ABArc[i], ABArc[i + 1], k);
	}
	triangle(ABArc[midpoint], c, d);
	triangle(ABArc[midpoint], d, k);
	for (int i = midpoint; i < 99; ++i)
	{
		triangle(ABArc[i], ABArc[i + 1], c);
	}

	triangle(d, e, k);
	triangle(e, g, k);
	triangle(e, f, g);

	for (int i = 0; i < 99; ++i)
	{
		triangle(O1, FGArc[i], FGArc[i + 1]);
	}

	//top
	/*turn_point = point(cos(total_turn), sin(total_turn), 0);

	for (int i = 0; i < midpoint; ++i)
	{
		triangle(ABArc[i].turn(turn_point), k.turn(turn_point), ABArc[i + 1].turn(turn_point), height);
	}
	triangle(ABArc[midpoint].turn(turn_point), d.turn(turn_point), c.turn(turn_point), height);
	triangle(ABArc[midpoint].turn(turn_point), k.turn(turn_point), d.turn(turn_point), height);

	for (int i = midpoint; i < 99; ++i)
	{
		triangle(ABArc[i].turn(turn_point), c.turn(turn_point), ABArc[i + 1].turn(turn_point), height);
	}

	triangle(d.turn(turn_point), k.turn(turn_point), e.turn(turn_point), height);
	triangle(e.turn(turn_point), k.turn(turn_point), g.turn(turn_point), height);
	triangle(e.turn(turn_point), g.turn(turn_point), f.turn(turn_point), height);

	for (int i = 0; i < 99; ++i)
	{
		triangle(O1.turn(turn_point), FGArc[i + 1].turn(turn_point), FGArc[i].turn(turn_point), height);
	}*/


	//walls

	int curve_num = 100;

	turn_point = point(cos(total_turn / curve_num), sin(total_turn / curve_num), 0);
	for (int j = 0; j < curve_num; ++j)
	{
		glColor3d(0.8 * j / curve_num, 0, 0.8 * j / curve_num);

		//AB arc
		for (int i = 0; i < curve_num - 1; ++i)
		{
			triangle(ABArc[i], ABArc[i + 1].turn(turn_point) + height / curve_num, ABArc[i + 1], height / curve_num * j);
			triangle(ABArc[i], ABArc[i].turn(turn_point) + height / curve_num, ABArc[i + 1].turn(turn_point) + height / curve_num, height / curve_num * j);
			ABArc[i] = ABArc[i].turn(turn_point);
		}
		ABArc[curve_num - 1] = ABArc[curve_num - 1].turn(turn_point);

		//FG arc
		for (int i = 0; i < curve_num - 1; ++i)
		{
			triangle(FGArc[i], FGArc[i + 1].turn(turn_point) + height / curve_num, FGArc[i + 1], height / curve_num * j);
			triangle(FGArc[i], FGArc[i].turn(turn_point) + height / curve_num, FGArc[i + 1].turn(turn_point) + height / curve_num, height / curve_num * j);
			FGArc[i] = FGArc[i].turn(turn_point);
		}
		FGArc[curve_num - 1] = FGArc[curve_num - 1].turn(turn_point);
	}
	int num = 40;
	turn_point = point(cos(total_turn / num), sin(total_turn / num), 0);
	for (int j = 0; j < num; ++j)
	{
		glColor3d(0.8 * j / num, 0, 0.8 * j / num);


		//everything else
		turned_quad(b, c, height, turn_point, num, height / num * j);
		turned_quad(c, d, height, turn_point, num, height / num * j);
		turned_quad(d, e, height, turn_point, num, height / num * j);
		turned_quad(e, f, height, turn_point, num, height / num * j);
		turned_quad(g, k, height, turn_point, num, height / num * j);
		turned_quad(k, a, height, turn_point, num, height / num * j);
		a = a.turn(turn_point);
		b = b.turn(turn_point);
		c = c.turn(turn_point);
		d = d.turn(turn_point);
		e = e.turn(turn_point);
		f = f.turn(turn_point);
		g = g.turn(turn_point);
		k = k.turn(turn_point);

	}
	glEnd();

	//top
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, texId);
	//glColor4d(0.75, 0, 0, 0.8);
	glBegin(GL_TRIANGLES);
	//reseting start values
	ld scale = 9;
	a = a1, b = b1, c = c1, d = d1, e = e1, f = f1, g = g1, k = k1;
	ABArc = { a };
	turn_point = point(cos(alpha / 99), sin(alpha / 99), 0);
	for (int i = 0; i < 99; ++i)
	{
		ABArc.push_back((ABArc[i] - O2).turn(turn_point) + O2);
	}
	FGArc = { f };
	turn_point = point(cos(beta / 99), -sin(beta / 99), 0);
	for (int i = 0; i < 99; ++i)
	{
		FGArc.push_back((FGArc[i] - O1).turn(turn_point) + O1);
	}

	turn_point = point(cos(total_turn), sin(total_turn), 0);

	for (int i = 0; i < midpoint; ++i)
	{
		texture_triangle(ABArc[i].turn(turn_point), k.turn(turn_point), ABArc[i + 1].turn(turn_point), scale, height);
	}
	texture_triangle(ABArc[midpoint].turn(turn_point), d.turn(turn_point), c.turn(turn_point), scale, height);
	texture_triangle(ABArc[midpoint].turn(turn_point), k.turn(turn_point), d.turn(turn_point), scale, height);

	for (int i = midpoint; i < 99; ++i)
	{
		texture_triangle(ABArc[i].turn(turn_point), c.turn(turn_point), ABArc[i + 1].turn(turn_point), scale, height);
	}

	texture_triangle(d.turn(turn_point), k.turn(turn_point), e.turn(turn_point), scale, height);
	texture_triangle(e.turn(turn_point), k.turn(turn_point), g.turn(turn_point), scale, height);
	texture_triangle(e.turn(turn_point), g.turn(turn_point), f.turn(turn_point), scale, height);

	for (int i = 0; i < 99; ++i)
	{
		texture_triangle(O1.turn(turn_point), FGArc[i + 1].turn(turn_point), FGArc[i].turn(turn_point), scale, height);
	}
	glEnd();
	//glDisable(GL_BLEND);

}


void bezier_curve(point a, point b, int num)
{
	for (int i = 0; i <= num; ++i)
	{
		(a + (b - a) / num * i).AddThis();
	}
}

void bezier_curve(point a, point b, point c, int num)
{
	for (int i = 0; i <= num; ++i)
	{
		point p1 = (a + (b - a) / num * i), p2 = (b + (c - b) / num * i);
		(p1 + (p2 - p1) / num * i).AddThis();
	}
}

void bezier_curve(point a, point b, point c, point d, int num)
{
	for (int i = 0; i <= num; ++i)
	{
		point p1 = (a + (b - a) / num * i), p2 = (b + (c - b) / num * i), p3 = (c + (d - c) / num * i);
		point p12 = (p1 + (p2 - p1) / num * i), p23 = (p2 + (p3 - p2) / num * i);
		(p12 + (p23 - p12) / num * i).AddThis();
	}
}

point bezier_curve_point(point a, point b, point c, point d, ld t)
{
	return (a * (1 - t) * (1 - t) * (1 - t) + b * 3 * t * (1 - t) * (1 - t) + c * 3 * t * t * (1 - t) + d * t * t * t);
}

void hermite_curve(point p1, point v1, point p2, point v2,  int num)
{
	for (int i = 0; i <= num; ++i)
	{
		ld t = (ld)i / 100;
		point temp = p1 * (2 * t * t * t - 3 * t * t + 1) + p2 * (-2 * t * t * t + 3 * t * t) + v1 * (t * t * t - 2 * t * t + t) + v2 * (t * t * t - t * t);
		temp.AddThis();
	}
}

void curves()
{
	point a(0, 0, 0), b(10, 0, 10), c(10, 10, 0), d(0, 10, 10);
	glBegin(GL_LINE_STRIP);
	glColor3d(0, 0, 1);
	a.AddThis(), b.AddThis(), c.AddThis(), d.AddThis();
	glEnd();

	glBegin(GL_LINE_STRIP);
	glColor3d(0, 0, 0);
	bezier_curve(a, b, c, d, 100);
	glEnd();


	glBegin(GL_LINES);
	glColor3d(1, 0, 0);
	a.AddThis(), (a + c).AddThis();
	b.AddThis(), (b + d).AddThis();
	glEnd();

	glBegin(GL_LINE_STRIP);
	glColor3d(0, 0, 0);
	hermite_curve(a, c, b, d, 100);
	glEnd();
}

void segment(point a, point b)
{
	a.AddThis();
	b.AddThis();
}

void draw_a_thing(point p, point v)
{
	ld size = 1;
	v.normalize();
	point vert = p + v * size;
	point v1(0, -1, 0);
	v1 = v1.turn(v);
	v1.normalize();
	segment(p, p + v1 * size);
	segment(p, p - v1 * size);
	segment(p, p + v * size * 1.5);
	segment(p, p + ((v1 * v) * size));
	segment(p, p - ((v1 * v) * size));
}

void line_move()
{
	point a(0, 0, 0), b(0, 10, 0), c(10, 10, 10), d(0, -10, 10);
	//-------------drawing of the lines----------------------------
	//curves();
	//-------------------------------------------------------------
	//------------drawing of the line and the moving thing---------
	glBegin(GL_LINE_STRIP);
	glColor3d(0, 0, 0);
	bezier_curve(a, b, c, d, 100);
	glEnd();

	glBegin(GL_LINES);
	static auto start = std::chrono::steady_clock::now();
	auto temp = std::chrono::steady_clock::now();
	std::chrono::duration<ld> time = temp - start;
	ld t = time.count() / 10;
	static int count = 1;
	if (t > 1)
	{
		if (t - count > 1)
		{
			++count;
		}
		t -= count;
	}
	point p = bezier_curve_point(a, b, c, d, t);
	point v = bezier_curve_point(a, b, c, d, t + 0.01) - p;
	draw_a_thing(p, v);
	glEnd();
	//------------------------------------------------------------
}

void Render(OpenGL *ogl)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); \
		//размер блика
		glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//======================================================================================================================
	//Прогать тут  

	line_move();

   //Сообщение вверху экрана

	
	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	                                //(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R="  << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	
	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}