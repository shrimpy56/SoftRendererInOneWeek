#include "Math.h"

Vector operator* (const Vector& v, double a)
{
	return Vector(a * v.x, a * v.y, a * v.z, 1);
}//todo,±êÁ¿×ó³Ë

Matrix4x4 operator * (double a, const Matrix4x4& m)
{
	Matrix4x4 ans;
	for (int i = 0; i < 4; ++i)
	for (int j = 0; j < 4; ++j)
	{
		ans.data[j][i] = a * m.data[i][j];
	}
	return ans;
}
Vector operator * (const Vector& v, const Matrix4x4& m)
{
	Vector ans;
	ans.x = v.x * m.data[0][0] + v.y * m.data[1][0] + v.z * m.data[2][0] + v.w * m.data[3][0];
	ans.y = v.x * m.data[0][1] + v.y * m.data[1][1] + v.z * m.data[2][1] + v.w * m.data[3][1];
	ans.z = v.x * m.data[0][2] + v.y * m.data[1][2] + v.z * m.data[2][2] + v.w * m.data[3][2];
	ans.w = v.x * m.data[0][3] + v.y * m.data[1][3] + v.z * m.data[2][3] + v.w * m.data[3][3];
	return ans;
}
Matrix4x4 getTranslateMatrix(double x, double y, double z)
{
	Matrix4x4 m;

	m.setIdentity();
	m.data[3][0] = x;
	m.data[3][1] = y;
	m.data[3][2] = z;

	return m;
}
Matrix4x4 getRotateMatrix(double x, double y, double z, double theta)
{
	Matrix4x4 m;

	double sinTheta = sin(theta * 3.1415926 / 180.0);
	double cosTheta = cos(theta * 3.1415926 / 180.0);
	Vector n = Vector(x, y, z, 1);
	n.normalize();
	m.setIdentity();
	m.data[0][0] = n.x * n.x * (1 - cosTheta) + cosTheta;
	m.data[0][1] = n.x * n.y * (1 - cosTheta) + n.z * sinTheta;
	m.data[0][2] = n.x * n.z * (1 - cosTheta) - n.y * sinTheta;
	m.data[1][0] = n.x * n.y * (1 - cosTheta) - n.z * sinTheta;
	m.data[1][1] = n.y * n.y * (1 - cosTheta) + cosTheta;
	m.data[1][2] = n.y * n.z * (1 - cosTheta) + n.x * sinTheta;
	m.data[2][0] = n.x * n.z * (1 - cosTheta) + n.y * sinTheta;
	m.data[2][1] = n.y * n.z * (1 - cosTheta) - n.x * sinTheta;
	m.data[2][2] = n.z * n.z * (1 - cosTheta) + cosTheta;

	return m;
}
Matrix4x4 getScaleMatrix(double x, double y, double z)
{
	Matrix4x4 m;

	m.setIdentity();
	m.data[0][0] = x;
	m.data[1][1] = y;
	m.data[2][2] = z;

	return m;
}

Matrix4x4 getViewMatrix(Vector eye, Vector center, Vector up)
{

	Vector x, y, z;
	z = center - eye;
	z.normalize();
	x = up.crossProduct(z);
	x.normalize();
	y = z.crossProduct(x);

	Matrix4x4 m;
	m.setIdentity();

	m.data[0][0] = x.x;
	m.data[1][0] = x.y;
	m.data[2][0] = x.z;
	m.data[3][0] = -x * eye;

	m.data[0][1] = y.x;
	m.data[1][1] = y.y;
	m.data[2][1] = y.z;
	m.data[3][1] = -y * eye;

	m.data[0][2] = z.x;
	m.data[1][2] = z.y;
	m.data[2][2] = z.z;
	m.data[3][2] = -z * eye;

	return m;
}

Matrix4x4 getPerspectiveMatrix(double fovy, double aspect, double zn, double zf)
{
	Matrix4x4 m;

	double zoomy = 1 / tan(fovy * 3.1415926 / 180.0 / 2);
	m.data[0][0] = zoomy / aspect;
	m.data[1][1] = zoomy;
	m.data[2][2] = zf / (zf - zn);
	m.data[3][2] = zn * zf / (zn - zf);
	m.data[2][3] = 1;

	return m;
}