#include <windows.h>
#include <iostream>
#include "Math.h"
#ifndef NDEBUG
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#endif
using namespace std;

const int Width = 800;
const int Height = 600;
unsigned char * ptr;
double zBuffer[Height][Width];
Camera mainCamera = { Vector(3, 3, 3, 1), Vector(), Vector(0, 1, 0, 1) };
double moveDelta = 0.1f;
const int texWidth = 32;
const int texHeight = 32;
unsigned int texture[texWidth][texHeight];
vector<Vertex> mesh;
enum RenderMode{
	WireFrame,//线框模式
	ColorShade,//无纹理的着色
	TextureMapping,//纹理映射
	RM_COUNT,
};
RenderMode renderMode = TextureMapping;
//光照参数
Color ambientLightColor(20, 20, 20);
Color directionalLightColor(255, 255, 255);
Vector directionalLightForward(-1, -1, -1, 1);

void DrawPixel(int x, int y, Color color, double z)
{
	if (x < 0 || x >= Width || y < 0 || y >= Height) return;
	if (z < zBuffer[y][x]) return;
	zBuffer[y][x] = z;
	unsigned int* iPtr = reinterpret_cast<unsigned int*>(ptr);
	iPtr[y * Width + x] = color.toUInt();
}

//DDA
void DrawLine(Vertex& v1, Vertex& v2)
{
	int x1 = int(v1.pos.x + 0.5), y1 = int(v1.pos.y+0.5), x2 = int(v2.pos.x+0.5), y2 = int(v2.pos.y+0.5);
	Color color1 = v1.color, color2 = v2.color;
	double z1 = v1.pos.z, z2 = v2.pos.z;
	if (x1 == x2)
	{
		if (y1 > y2)
		{
			swap(y1, y2);
			swap(color1, color2);
			swap(z1, z2);
		}
		for (int i = y1; i <= y2; ++i)
		{
			double percent = (y1 == y2 ? 0.5 : (i - y1) * 1.0 / (y2 - y1));
			DrawPixel(x1, i, color1.interpolate(color2, percent), z1 + (z2 - z1)*percent);
		}
	}
	else if (y1 == y2)
	{
		if (x1 > x2)
		{
			swap(x1, x2);
			swap(color1, color2);
			swap(z1, z2);
		}
		for (int i = x1; i <= x2; ++i)
		{
			double percent = (x1 == x2 ? 0.5 : (i - x1) * 1.0 / (x2 - x1));
			DrawPixel(i, y1, color1.interpolate(color2, percent), z1 + (z2 - z1)*percent);
		}
	}
	else
	{
		int dx = x2 - x1;
		if (dx < 0) dx = -dx;
		int dy = y2 - y1;
		if (dy < 0) dy = -dy;
		if (dx >= dy) {
			if (x1 > x2)
			{
				swap(x1, x2);
				swap(y1, y2);
				swap(z1, z2);
				swap(color1, color2);
			}
			double m = (y2 - y1) * 1.0f / (x2 - x1);
			double y = y1;
			for (int i = x1; i <= x2; ++i)
			{
				double percent = (x1 == x2 ? 0 : (i - x1) * 1.0 / (x2 - x1));
				DrawPixel(i, int(y + 0.5), color1.interpolate(color2, percent), z1 + (z2 - z1)*percent);
				y += m;
			}
		}
		else {
			if (y1 > y2)
			{
				swap(x1, x2);
				swap(y1, y2);
				swap(z1, z2);
				swap(color1, color2);
			}
			double m = (x2 - x1) * 1.0f / (y2 - y1);
			double x = x1;
			for (int i = y1; i <= y2; ++i)
			{
				double percent = (y1 == y2 ? 0 : (i - y1) * 1.0 / (y2 - y1));
				DrawPixel(int(x + 0.5), i, color1.interpolate(color2, percent), z1 + (z2 - z1)*percent);
				x += m;
			}
		}
	}
}

//顶点变换
void vertexTransform(Vertex& vertex)
{
	Vector& v = vertex.pos;
	Matrix4x4 modelViewMatrix = getViewMatrix(mainCamera.eye, mainCamera.center, mainCamera.up);
	Matrix4x4 projectionMatrix = getPerspectiveMatrix(45, Width / Height, 0.2, 20);
	v = v * modelViewMatrix * projectionMatrix;
	//漫反射
	Color finalColor = ambientLightColor;
	finalColor += (renderMode == TextureMapping ? Color(255, 255, 255) : vertex.color) * directionalLightColor * max(0, -directionalLightForward.normalize() * vertex.normal.normalize());
	vertex.color = finalColor;
	v.toNDC();
	vertex.u /= v.w;
	vertex.v /= v.w;
}

void vertexPostProcess(Vertex& vertex)
{
	Vector& v = vertex.pos;
	v.z = 1 / v.w;//1/z buffer
	v.x = (v.x + 1) * Width / 2.0f;
	v.y = (1 - v.y) * Height / 2.0f;
}

//Gourand着色
void scanLine(Vertex v1, Vertex v2, Vertex v3)
{
	if (v1.pos.y > v2.pos.y)
	{
		swap(v1, v2);
	}
	if (v1.pos.y > v3.pos.y)
	{
		swap(v1, v3);
	}
	if (v2.pos.y > v3.pos.y)
	{
		swap(v2, v3);
	}
	for (int y = min(Height - 1, int(v3.pos.y + 0.5)); y >= max(0, int(v2.pos.y + 0.5)); --y)
	{
		double percentA = (float2int(v3.pos.y) == float2int(v2.pos.y)) ? 0 : (y - float2int(v3.pos.y)) * 1.0 / (float2int(v2.pos.y) - float2int(v3.pos.y));
		double percentB = (float2int(v3.pos.y) == float2int(v1.pos.y)) ? 0 : (y - float2int(v3.pos.y)) * 1.0 / (float2int(v1.pos.y) - float2int(v3.pos.y));
		Vertex a = v3.interpolate(v2, percentA);
		Vertex b = v3.interpolate(v1, percentB);
		Vector& posA = a.pos;
		Vector& posB = b.pos;
		Color& colA = a.color;
		Color& colB = b.color;
		if (posA.x > posB.x)
		{
			swap(a, b);
		}
		for (int x = max(0, int(posA.x+0.5)); x <= min(Width - 1, int(posB.x+0.5)); ++x)
		{
			double percent = (float2int(posB.x) == float2int(posA.x)) ? 0 : (x - float2int(posA.x)) * 1.0 / (float2int(posB.x) - float2int(posA.x));
			Vertex vP = a.interpolate(b, percent);
			Color c;
			if (renderMode == TextureMapping)
			{
				double u = vP.u / vP.pos.z * (texWidth - 1);
				clamp(u, 0, texWidth - 1);
				double v = vP.v / vP.pos.z * (texHeight - 1);
				clamp(v, 0, texHeight - 1);
				c = vP.color * Color(texture[float2int(u)][float2int(v)]);
			}
			else c = vP.color;
			DrawPixel(x, y, c, vP.pos.z);
		}
	}
	for (int y = min(Height-1, int(v2.pos.y+0.5)); y >= max(0, int(v1.pos.y+0.5)); --y)
	{
		double percentA = (float2int(v2.pos.y) == float2int(v1.pos.y)) ? 0 : (y - float2int(v2.pos.y)) * 1.0 / (float2int(v1.pos.y) - float2int(v2.pos.y));
		double percentB = (float2int(v3.pos.y) == float2int(v1.pos.y)) ? 0 : (y - float2int(v3.pos.y)) * 1.0 / (float2int(v1.pos.y) - float2int(v3.pos.y));
		Vertex a = v2.interpolate(v1, percentA);
		Vertex b = v3.interpolate(v1, percentB);
		Vector& posA = a.pos;
		Vector& posB = b.pos;
		Color& colA = a.color;
		Color& colB = b.color;
		if (posA.x > posB.x)
		{
			swap(a, b);
		}
		for (int x = max(0, int(posA.x+0.5)); x <= min(Width - 1, int(posB.x+0.5)); ++x)
		{
			double percent = (float2int(posB.x) == float2int(posA.x)) ? 0 : (x - float2int(posA.x)) * 1.0 / (float2int(posB.x) - float2int(posA.x));
			Vertex vP = a.interpolate(b, percent);
			Color c;
			if (renderMode == TextureMapping)
			{
				double u = vP.u / vP.pos.z * (texWidth - 1);
				clamp(u, 0, texWidth - 1);
				double v = vP.v / vP.pos.z * (texHeight - 1);
				clamp(v, 0, texHeight - 1);
				c = vP.color * Color(texture[float2int(u)][float2int(v)]);
			}
			else c = vP.color;
			DrawPixel(x, y, c, vP.pos.z);
		}
	}
}

//裁剪三角形情形1,两个顶点需要裁剪
void clipTriangle1(Vertex& v0, Vertex& v1, Vertex& v2)
{
	double t = (EPS - v0.pos.z) / (v2.pos.z - v0.pos.z);
	v2 = v0.interpolate(v2, t);

	t = (EPS - v0.pos.z) / (v1.pos.z - v0.pos.z);
	v1 = v0.interpolate(v1, t);
}

//裁剪三角形情形2，一个顶点要裁剪
void clipTriangle2(Vertex& v0, Vertex& v1, Vertex& v2)
{
	double t = (EPS - v0.pos.z) / (v2.pos.z - v0.pos.z);
	Vertex nv2 = v0.interpolate(v2, t);

	t = (EPS - v0.pos.z) / (v1.pos.z - v0.pos.z);
	v0 = v0.interpolate(v1, t);

	mesh.push_back(v0);
	mesh.push_back(v2);
	mesh.push_back(nv2);
}

//绘制三角形
void drawTriangle(Vertex v1, Vertex v2, Vertex v3, bool withOutTrans = false)
{
	if (!withOutTrans)
	{
		vertexTransform(v1);
		vertexTransform(v2);
		vertexTransform(v3);
	}

	//远平面上用简单的CVV裁剪
	if (v1.pos.z > 1 || v2.pos.z > 1 || v3.pos.z > 1) return;

	//在近平面上分割三角形裁剪
	bool clipV1 = v1.pos.z < 0;
	bool clipV2 = v2.pos.z < 0;
	bool clipV3 = v3.pos.z < 0;
	int num = clipV1 + clipV2 + clipV3;
	switch (num)
	{
	case 1:
		if (clipV1)
		{
			clipTriangle2(v1, v2, v3);
		}
		else if (clipV2)
		{
			clipTriangle2(v2, v3, v1);
		}
		else if (clipV3)
		{
			clipTriangle2(v3, v1, v2);
		}
		break;
	case 2:
		if (!clipV1)
		{
			clipTriangle1(v1, v2, v3);
		}
		else if (!clipV2)
		{
			clipTriangle1(v2, v3, v1);
		}
		else if (!clipV3)
		{
			clipTriangle1(v3, v1, v2);
		}
		break;
	case 3:
		return;
	}

	vertexPostProcess(v1);
	vertexPostProcess(v2);
	vertexPostProcess(v3);

	//是否线框
	if (renderMode != WireFrame)
	{
		scanLine(v1, v2, v3);
	}
	else
	{
		DrawLine(v1, v2);
		DrawLine(v1, v3);
		DrawLine(v2, v3);
	}
}

//渲染
void display()
{
	//box
	Vertex v1{ { 1, -1, 1, 1 }, 0, 0, { 255, 255, 255 }, { 1, -1, 1, 1 } };
	Vertex v2{ { -1, -1, 1, 1 }, 1, 0, { 0, 255, 255 }, { -1, -1, 1, 1 } };
	Vertex v3{ { -1, 1, 1, 1 }, 1, 1, { 255, 0, 255 }, { -1, 1, 1, 1 } };
	Vertex v4{ { 1, 1, 1, 1 }, 0, 1, { 255, 255, 0 }, { 1, 1, 1, 1 } };
	Vertex v5{ -v3.pos, v3.u, v3.v, v3.color, -v3.normal };
	Vertex v6{ -v4.pos, v4.u, v4.v, v4.color, -v4.normal };
	Vertex v7{ -v1.pos, v1.u, v1.v, v1.color, -v1.normal };
	Vertex v8{ -v2.pos, v2.u, v2.v, v2.color, -v2.normal };
	//plane
	Vertex t1{ { 2, 0, 2, 1 }, 0, 0, { 255, 255, 255 }, { 0, 1, 0, 1 } };
	Vertex t2{ { 3, 0, 2, 1 }, 1, 0, { 0, 255, 255 }, { 0, 1, 0, 1 } };
	Vertex t3{ { 2, 1, 2, 1 }, 1, 1, { 255, 0, 255 }, { 0, 1, 0, 1 } };

	mesh = {
		v1, v3, v2,
		v1, v4, v3,
		v6, v8, v5,
		v6, v7, v8,
		v5.setUV(0, 0), v8.setUV(0, 1), v4.setUV(1, 1),
		v5, v4, v1.setUV(1, 0),
		v2.setUV(v8.u, v8.v), v3.setUV(v5.u, v5.v), v7.setUV(v1.u, v1.v),
		v2, v7, v6.setUV(v4.u, v4.v),
		v7.setUV(0, 0), v3.setUV(0, 1), v4.setUV(1, 1),
		v7, v4, v8.setUV(1, 0),
		v6.setUV(v4.u, v4.v), v2.setUV(v8.u, v8.v), v1.setUV(v7.u, v7.v),
		v6, v1, v5.setUV(v3.u, v3.v),
		t1, t3, t2,
	};
	//int length = sizeof (mesh) / sizeof (Vertex);
	int i = 0;
	int length = mesh.size();
	for (; i < length; i += 3)
	{
		drawTriangle(mesh[i], mesh[i+1], mesh[i+2]);
	}
	for (; i < mesh.size(); i += 3)
	{
		drawTriangle(mesh[i], mesh[i + 1], mesh[i + 2], true);
	}
}

//生成纹理
void initTexture()
{
	for (int i = 0; i < texWidth; ++i)
		for (int j = 0; j < texHeight; ++j)
		{
			if ((i / 4 + j / 4) & 1)
				texture[i][j] = 0x000000;
			else
				texture[i][j] = 0xffffff;
		}
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		//上下左右控制摄像机前后和旋转
		switch (wparam)
		{
		case VK_UP:
			{
				Vector f = mainCamera.center - mainCamera.eye;
				if (f.length() > moveDelta)
					mainCamera.eye += f.normalize() * moveDelta;
			}
			break;
		case VK_DOWN:
			mainCamera.eye -= (mainCamera.center - mainCamera.eye).normalize() * moveDelta;
			break;
		case VK_LEFT:
			{
				Vector x, y, z;
				z = mainCamera.center - mainCamera.eye;
				z.normalize();
				x = mainCamera.up.crossProduct(z);
				x.normalize();
				y = z.crossProduct(x);
				mainCamera.eye = mainCamera.eye * getRotateMatrix(y.x, y.y, y.z, moveDelta*10);
				mainCamera.up = y;
			}
			break;
		case VK_RIGHT:
			{
				 Vector x, y, z;
				 z = mainCamera.center - mainCamera.eye;
				 z.normalize();
				 x = mainCamera.up.crossProduct(z);
				 x.normalize();
				 y = z.crossProduct(x);
				 mainCamera.eye = mainCamera.eye * getRotateMatrix(y.x, y.y, y.z, -moveDelta*10);
				 mainCamera.up = y;
			}
			break;
		case VK_SPACE:
			renderMode = RenderMode((renderMode + 1) % RM_COUNT);
			break;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// 打开控制台
bool CreateConsole(void)
{
#ifndef NDEBUG
	FreeConsole();
	if (AllocConsole())
	{
		int hCrt = _open_osfhandle((long)
			GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
		*stdout = *(::_fdopen(hCrt, "w"));
		setvbuf(stdout, NULL, _IONBF, 0);
		*stderr = *(::_fdopen(hCrt, "w"));
		setvbuf(stderr, NULL, _IONBF, 0);
		return TRUE;
	}
#endif
	return FALSE;
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE hprevinstance,
	LPSTR lpcmdline,
	int nshowcmd)
{
	MSG msg;
	WNDCLASSEX ex;

	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ex.lpfnWndProc = WinProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinstance;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	ex.lpszMenuName = NULL;
	ex.lpszClassName = L"wndclass";
	ex.hIconSm = NULL;

	RegisterClassEx(&ex);

	// Create the window
	HWND hwnd = CreateWindowEx(NULL,
		L"wndclass",
		L"Window",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0,
		Width, Height,
		NULL,
		NULL,
		hinstance,
		NULL);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// Debug console
	CreateConsole();

	HDC hdc = GetDC(hwnd);
	HDC buffer_hdc = CreateCompatibleDC(hdc);
	BITMAPINFO info = { sizeof(BITMAPINFOHEADER), Width, -Height, 1, 32, BI_RGB,
		Width * Height * 4, 0, 0, 0, 0 };
	HBITMAP dibSection = CreateDIBSection(buffer_hdc, &info, DIB_RGB_COLORS, reinterpret_cast<void **>(&ptr), 0, 0);
	SelectObject(buffer_hdc, dibSection);
	
	initTexture();

	bool quit = false;
	// The message loop
	while (!quit)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				quit = true;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		memset(ptr, 0, Width * Height * 4);
		memset(zBuffer, 0, sizeof(zBuffer));
		display();
		BitBlt(hdc, 0, 0, Width, Height, buffer_hdc, 0, 0, SRCCOPY);
	}

	ReleaseDC(hwnd, hdc);
	return msg.lParam;
}