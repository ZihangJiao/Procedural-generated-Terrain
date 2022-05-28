#include "pch.h"
#include "Ray.h"
#include <SimpleMath.h>


//Ray::Ray() {
//
//	m_near = 10.0f;
//	m_far = 4000.0f;
//	m_fov = 0.8f;
//	m_width = 800.0f;
//	m_height = 600.0f;
//	m_width_div2 = m_width / 2;
//	m_height_div2 = m_height / 2;
//	m_aspect = 1.3333f;
//
//
//};
//Ray::~Ray() {};
//
//void Ray::calcRay(int x, int y, DirectX::SimpleMath::Vector3& p1, DirectX::SimpleMath::Vector3& p2) {
//	float dx, dy;
//	DirectX::SimpleMath::Matrix intMatrix, viewMatrix;
//	dx = tanf(m_fov * 0.5f) * (x / m_width_div2 - 1.0f) / m_aspect;
//	dy = tanf(m_fov * 0.5f) * (1.0f - y / m_height_div2);
//	lpDevice->GetTransform(TRANSFORMSTATE_VIEW, &viewMatrix);
//	Math_MatrixInvert(invMatrix, viewMatrix);
//	p1 = DirectX::SimpleMath::Vector3(dx * NEAR, dy * NEAR, NEAR);
//	p2 = DirectX::SimpleMath::Vector3(dx * FAR, dy * FAR, FAR);
//
//	D3DMath_VectorMatrixMultiply(p1, p1, invMatrix);
//	D3DMath_VectorMatrixMultiply(p2, p2, invMatrix);
//
//};
//
//DirectX::SimpleMath::Vector3 Ray::calDir(int x, int y) {
//	DirectX::SimpleMath::Vector3 p1, p2;
//	calcRay(x, y, p1, p2);
//	return 
//};