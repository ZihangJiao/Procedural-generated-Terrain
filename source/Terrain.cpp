#include "pch.h"
#include "Terrain.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static float p[B + B + 2];


#define s_curve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( a + t * (b - a) )



static int start = 1;

static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];

Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
}


Terrain::~Terrain()
{
}

bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight)
{
	int index;
	float height = 0.0;
	bool result;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1;
	m_deathLimit = 4;
	m_birthLimit = 4;
	m_aliveProb = 50;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	m_aliveMap = new bool[m_terrainWidth * m_terrainHeight];
	InitiliseAliveStatus(device);

	if (!m_heightMap)
	{
		return false;
	}

	//this is how we calculate the texture coordinates first calculate the step size there will be between vertices. 
	float textureCoordinatesStep = 1.0f / m_terrainWidth;  //tile 5 times across the terrain. 
	// Initialise the data in the height map (flat).
	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

			//and use this step to calculate the texture coordinates for this point on the terrain.
			m_heightMap[index].u = (float)i * textureCoordinatesStep;
			m_heightMap[index].v = (float)j * textureCoordinatesStep;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	
	return true;
}

void Terrain::Render(ID3D11DeviceContext * deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->Draw(m_indexCount, 0);

	return;
}

void Terrain::IndexRender(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);
	//deviceContext->Draw(m_vertexCount, 0);
	return;
}


bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	DirectX::SimpleMath::Vector3* normals;
	

	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new DirectX::SimpleMath::Vector3[(m_terrainHeight - 1) * (m_terrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (j * m_terrainHeight) + i;
			index2 = (j * m_terrainHeight) + (i + 1);
			index3 = ((j + 1) * m_terrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;

			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;

			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainHeight - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<m_terrainHeight; j++)
	{
		for (i = 0; i<m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_terrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_terrainWidth - 1)) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

bool Terrain::InitializeBuffers(ID3D11Device * device )
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;

	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainHeight * j) + i;          // Bottom left.
			index2 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
			index3 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
			index4 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.

															 // Upper left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
			indices[index] = index;
			index++;
		}
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void Terrain::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}


void Terrain::GenerateRandomHeightField() {

}

bool Terrain::Smooth(ID3D11Device* device) {
	bool result;

	int index;
	float height = 0.0;

	m_frequency = (6.283 / m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283
	if (m_amplitude > 0) {
		m_amplitude -= 0.01;
	}

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

		//	m_heightMap[index].x = (float)i;
		//	//m_heightMap[index].y = (float)(sin((float)i *(m_frequency))*m_amplitude); 
		//	float num = (float)(sin((float)j * (m_frequency)) * m_amplitude);
		///*	if (num < 0) {
		//		m_heightMap[index].y = num + 0.5;
		//	}if (num > 0) {
		//		m_heightMap[index].y = num - 0.5;
		//	}*/
		//	m_heightMap[index].y = num;
		//	m_heightMap[index].z = (float)j;

			m_heightMap[index].x = (float)i;
			
			m_heightMap[index].y = (float)((rand()%100* (m_frequency)) * m_amplitude)/20;
		//	m_heightMap[index].y = PerlinNoise(float(i),float(j));

			m_heightMap[index].z = (float)j;


		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}


bool Terrain::PerlinNoise(ID3D11Device* device) {
	// Properties

	m_frequency = (6.283 / m_terrainHeight) / m_wavelength;
	const int octaves = 1;
	float lacunarity = 2.0;
	float gain = 0.5;
	//
	// 
	float total = 0;
	int index;

	float ampli = m_amplitude;
	float freq = m_frequency;

	// Loop of octaves
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++) {
			index = (m_terrainHeight * j) + i;
		/*	for (int k = 0; k < octaves; k++) {
				total += Perlin((float)i * m_frequency, (float)j * m_frequency) * m_amplitude;
				m_frequency *= lacunarity;
				m_amplitude *= gain;
				m_heightMap[index].x = (float)i;
				m_heightMap[index].y = total;
				m_heightMap[index].z = (float)j;
			}
			m_amplitude = ampli;
			m_frequency = freq;
			total = 0;*/

			total = Perlin((float)i * m_frequency, (float)j * m_frequency)* m_amplitude*20;
			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = total;
			m_heightMap[index].z = (float)j;
		}
		bool result;
		result = CalculateNormals();
		if (!result)
		{
			return false;
		}

		result = InitializeBuffers(device);
		if (!result)
		{
			return false;
		}
	}

}

float Terrain::Perlin(float x, float y) {
	/*int n;
	n = x + y * 57;
	n = (n << 13) ^ n;
	return (1.0 - ((n * ((n * n * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);*/

	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, * q, sx, sy, a, b, t, u, v;
	register float i, j;

	if (start) {
		start = 0;
		init();
	}

float vec[2] = { x, y };
#define setup(i,b0,b1,r0,r1)\
t = vec[i] + N; \
b0 = ((int)t) & BM; \
b1 = (b0 + 1) & BM; \
r0 = t - (int)t; \
r1 = r0 - 1.;






	setup(0, bx0, bx1, rx0, rx1);
	setup(1, by0, by1, ry0, ry1);

	i = p[bx0];
	j = p[bx1];

	b00 = p[(int)i + by0];
	b10 = p[(int)j + by0];
	b01 = p[(int)i + by1];
	b11 = p[(int)j + by1];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

	q = g2[b00]; u = at2(rx0, ry0);
	q = g2[b10]; v = at2(rx1, ry0);
	a = lerp(sx, u, v);

	q = g2[b01]; u = at2(rx0, ry1);
	q = g2[b11]; v = at2(rx1, ry1);
	b = lerp(sx, u, v);

	return lerp(sy, a, b);


}



bool Terrain::CellularAutomata(ID3D11Device* device){
	bool result;
	int index;
	int deathLimit = m_deathLimit;
	int birthLimit = m_birthLimit;

	bool* temp_aliveMap = m_aliveMap;

	// Loop of octaves
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++) {
			int nbs = CountNeighbors(m_aliveMap, j, i);

			index = (m_terrainHeight * j) + i;
			if (m_aliveMap[index] == true) {
				//m_heightMap[index].y /= 2;
				if (nbs < deathLimit) {
					//m_aliveMap[index] = false;
					temp_aliveMap[index] = false;
					m_heightMap[index].y = 0.0;
					//m_heightMap[index].y = 0;
				}
				else {
					//m_aliveMap[index] = true;
					temp_aliveMap[index] = true;
					m_heightMap[index].y = -2.0;
					//m_heightMap[index].y *= 2;
					//m_heightMap[index].y = 10;
				}
			}
			else {
				//m_heightMap[index].y = 50.0;
				if (nbs > birthLimit) {
					//m_aliveMap[index] = true;
					temp_aliveMap[index] = true;
					m_heightMap[index].y = -2.0;
					//m_heightMap[index].y *= 2;
					//m_heightMap[index].y = 10;
				}
				else {
					//m_aliveMap[index] = false;
					temp_aliveMap[index] = false;
					m_heightMap[index].y = 0.0;
					//m_heightMap[index].y = 0;
				}
			}

		}
	}

	m_aliveMap = temp_aliveMap;
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

}




bool Terrain::ConwayGame(ID3D11Device* device) {
	bool result;
	int index;

	bool* temp_aliveMap = m_aliveMap;
	// Loop of octaves
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++) {
			int nbs = CountNeighbors(m_aliveMap, j, i);

			index = (m_terrainHeight * j) + i;
			if (m_aliveMap[index] == true) {
				//m_heightMap[index].y /= 2;
				if (nbs < 2) {
					temp_aliveMap[index] = false;
					//m_aliveMap[index] = false;
					m_heightMap[index].y = 0.0;
					//m_heightMap[index].y = 0;
				}
				else if(nbs == 2 || nbs == 3){
					temp_aliveMap[index] = true;
					//m_aliveMap[index] = true;
					m_heightMap[index].y = -2.0;
					//m_heightMap[index].y *= 2;
					//m_heightMap[index].y = 10;
				}
				else if (nbs > 3) {
					temp_aliveMap[index] = false;
					//m_aliveMap[index] = false;
					m_heightMap[index].y = 0.0;
				}
			}
			else {
				//m_heightMap[index].y = 50.0;
				if (nbs == 3) {
					temp_aliveMap[index] = true;
					//m_aliveMap[index] = true;
					m_heightMap[index].y = -2.0;
					//m_heightMap[index].y *= 2;
					//m_heightMap[index].y = 10;
				}
			}

		}
	}
	m_aliveMap = temp_aliveMap;

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}




bool Terrain::InitiliseAliveStatus(ID3D11Device* device) {
	bool result;
	int index;
	int random_alive_prob = m_aliveProb;
	// create random alive state map
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;
			auto random = rand() % 100;
			if (random < random_alive_prob) {
				m_aliveMap[index] = true;
				m_heightMap[index].y = -2.0;
			}

			else {
				m_aliveMap[index] = false;
				m_heightMap[index].y = 0.0;
			}

			//m_heightMap[index].y = 0.1;

		}
	}
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

}


int Terrain::CountNeighbors(bool*& alive_map, int x, int y) {

	int count = 0;
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {// go through four directions
			int neighbourX = x + i;
			int neighbourY = y + j;
			int index = (m_terrainHeight * neighbourX) + neighbourY;
			if (i == 0 && j == 0)
				continue;
			else if (neighbourX < 0 || neighbourY < 0 || neighbourX >= m_terrainHeight || neighbourY >= m_terrainWidth)
				count = count + 1;

			else if (alive_map[index]) {
				count = count + 1;
			}
		}
			
	}
	return count;

}




void Terrain::normalize2(float v[2])
{
	float s;

	s = sqrt(v[0] * v[0] + v[1] * v[1]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
}

void Terrain:: normalize3(float v[3])
{
	float s;

	s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] = v[0] / s;
	v[1] = v[1] / s;
	v[2] = v[2] / s;
}

void Terrain::init()
{
	int i, j, k;

	for (i = 0; i < B; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0; j < 2; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize2(g2[i]);

		for (j = 0; j < 3; j++)
			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize3(g3[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0; i < B + 2; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0; j < 2; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0; j < 3; j++)
			g3[B + i][j] = g3[i][j];
	}
}



bool Terrain::GenerateHeightMap(ID3D11Device* device)
{
	bool result;

	int index;
	float height = 0.0;

	m_frequency = (6.283/m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			//m_heightMap[index].y = (float)(sin((float)i *(m_frequency))*m_amplitude); 
			m_heightMap[index].y = (float)(sin((float)j * (m_frequency)) * m_amplitude);
			m_heightMap[index].z = (float)j;

			//m_heightMap[index].x = (float)i;
			////m_heightMap[index].y =(float)(sin((float)i *(m_frequency))*m_amplitude); 
			//m_heightMap[index].y = (float)((rand()%100* (m_frequency)) * m_amplitude)/20;

			//m_heightMap[index].z = (float)j;

		
		}
	}











	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

bool Terrain::Update()
{
	return true; 
}

float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}

int* Terrain::GetDeathLimit()
{
	return &m_deathLimit;
}

int* Terrain::GetBirthLimit()
{
	return &m_birthLimit;
}

int* Terrain::GetAliveProb()
{
	return &m_aliveProb;
}