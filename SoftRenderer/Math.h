#ifndef MyMath
#define MyMath
#include <vector>
#include <iostream>
using namespace std;

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define clamp(a, mi, ma) ((a < mi) ? mi : (a > ma ? ma : a))
#define float2int(f) (int(f+0.5))
const double EPS = 1e-4;

struct Vector{
	friend Vector operator* (const Vector& v, double a);
	double x, y, z, w;
	Vector(){ x = y = z = w = 0; }
	Vector(double x, double y, double z, double w = 0) : x(x), y(y), z(z), w(w){}
	Vector& normalize()
	{
		double squareMag = x * x + y * y + z * z;
		if (squareMag > EPS)
		{
			double sq = sqrt(squareMag);
			x /= sq;
			y /= sq;
			z /= sq;
		}
		return *this;
	}
	double length()
	{
		return sqrt(x * x + y * y + z * z);
	}
	void toNDC()
	{
		if (w > -EPS && w < EPS)
		{
			cerr << "w is 0";
			return;
		}
		x /= w;
		clamp(x, -1, 1);
		y /= w;
		clamp(y, -1, 1);
		z /= w;
		clamp(z, 0, 1);
	}
	Vector interpolate(const Vector& to, double percent)
	{
		percent = clamp(percent, 0, 1);
		return Vector(x + (to.x - x) * percent, y + (to.y - y) * percent, z + (to.z - z) * percent, w);
	}
	Vector operator + (const Vector& v) const
	{
		return Vector(x + v.x, y + v.y, z + v.z, 1);
	}
	Vector& operator += (const Vector& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w = 1;
		return *this;
	}
	Vector operator - (const Vector& v) const
	{
		return Vector(x - v.x, y - v.y, z - v.z, 1);
	}
	Vector operator - () const
	{
		return (*this * -1);
	}
	Vector& operator -= (const Vector& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w = 1;
		return *this;
	}
	//µã»ý
	double operator * (const Vector& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}
	//²æ»ý
	Vector crossProduct(const Vector& v) const
	{
		return Vector(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x, 1);
	}
	Vector operator / (double a)
	{
		return Vector(x / a, y / a, z / a, 1);
	}
};
Vector operator* (const Vector& v, double a);

struct Matrix4x4{
	double data[4][4];
	Matrix4x4(){ resetToZero(); }
	Matrix4x4(const vector<double>& m)//todo,m[3][3]
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
			{
				data[i][j] = m[i * 4 + j];
			}
	}
	void resetToZero()
	{
		memset(data, 0, sizeof(data));
	}
	void setIdentity()
	{
		resetToZero();
		data[0][0] = data[1][1] = data[2][2] = data[3][3] = 1;
	}
	Matrix4x4 operator * (const Matrix4x4& m)
	{
		Matrix4x4 ans;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
			{
				ans.data[i][j] = 0;
				for (int k = 0; k < 4; ++k)
				{
					ans.data[i][j] += data[i][k] * m.data[k][j];
				}
			}
		return ans;
	}
};
Matrix4x4 operator * (double a, const Matrix4x4& m);
Vector operator * (const Vector& v, const Matrix4x4& m);
Matrix4x4 getTranslateMatrix(double x, double y, double z);
Matrix4x4 getRotateMatrix(double x, double y, double z, double theta);
Matrix4x4 getScaleMatrix(double x, double y, double z);
Matrix4x4 getViewMatrix(Vector eye, Vector center, Vector up);
Matrix4x4 getPerspectiveMatrix(double fovy, double aspect, double zn, double zf);

struct Color
{
	unsigned char r, g, b;
	Color():r(0), g(0), b(0){}
	Color(unsigned char r, unsigned char g, unsigned char b): r(r), g(g), b(b){}
	Color(unsigned int c)
	{
		r = c & 0x0000ff;
		g = (c & 0x00ff00) >> 8;
		b = c >> 16;
	}
	Color operator * (const Color& c)
	{
		Color ans;
		ans.r = static_cast<unsigned char>((r * 1.0 / 255) * (c.r * 1.0 / 255)* 255) ;
		ans.g = static_cast<unsigned char>((g * 1.0 / 255) * (c.g * 1.0 / 255)* 255) ;
		ans.b = static_cast<unsigned char>((b * 1.0 / 255) * (c.b * 1.0 / 255)* 255) ;
		return ans;
	}
	Color operator * (double num)
	{
		Color ans;
		ans.r = static_cast<unsigned char>(r * num);
		ans.g = static_cast<unsigned char>(g * num);
		ans.b = static_cast<unsigned char>(b * num);
		return ans;
	}
	Color operator + (const Color& c)
	{
		return Color(min(255, r + c.r), min(255, g + c.g), min(255, b + c.b));
	}
	Color& operator += (const Color& c)
	{
		r = min(255, r + c.r);
		g = min(255, g + c.g);
		b = min(255, b + c.b);
		return *this;
	}
	Color interpolate(const Color& to, double percent)
	{
		percent = clamp(percent, 0, 1);
		return Color(r + static_cast<unsigned char>((to.r - r) * percent), g + static_cast<unsigned char>((to.g - g) * percent), b + static_cast<unsigned char>((to.b - b) * percent));
	}
	unsigned int toUInt()
	{
		return unsigned int((r << 16) | (g << 8) | b);
	}
};

struct Vertex
{
	Vector pos;
	double u, v;
	Color color;
	Vector normal;
	Vertex interpolate(const Vertex& to, double percent)
	{
#ifndef NDEBUG
		if (percent < 0 || percent > 1) cout << percent << ":" << pos.x << "," << pos.y;
#endif
		percent = clamp(percent, 0, 1);
		Vertex ans;
		ans.pos = pos.interpolate(to.pos, percent);
		ans.u = u + (to.u - u) * percent;
		ans.v = v + (to.v - v) * percent;
		//cerr << u << "," << v << endl;
		ans.color = color.interpolate(to.color, percent);
		ans.normal = normal.interpolate(to.normal, percent);
		return ans;
	}
	Vertex& setUV(double u, double v)
	{
		this->u = u;
		this->v = v;
		return *this;
	}
};

struct Camera
{
	Vector eye, center, up;
};

#endif