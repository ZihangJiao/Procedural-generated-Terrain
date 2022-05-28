#pragma once

using namespace DirectX;

class Terrain
{
private:
	struct VertexType
	{
		DirectX::SimpleMath::Vector3 position;
		DirectX::SimpleMath::Vector2 texture;
		DirectX::SimpleMath::Vector3 normal;
	};
	struct HeightMapType
	{
		float x, y, z;
		float nx, ny, nz;
		float u, v;
	};


public:
	Terrain();
	~Terrain();

	bool Initialize(ID3D11Device*, int terrainWidth, int terrainHeight);
	void Render(ID3D11DeviceContext*);
	void IndexRender(ID3D11DeviceContext*);
	bool GenerateHeightMap(ID3D11Device*);
	void GenerateRandomHeightField();
	bool Smooth(ID3D11Device*);
	bool CellularAutomata(ID3D11Device*);
	bool ConwayGame(ID3D11Device*);

	float Perlin(float x, float y);
	bool PerlinNoise(ID3D11Device* device);

	void init();
	void normalize2(float v[2]);
	void normalize3(float v[3]);
	
	bool Update();
	float* GetWavelength();

	float* GetAmplitude();

	int* GetDeathLimit();

	int* GetBirthLimit();

	int* GetAliveProb();

	bool InitiliseAliveStatus(ID3D11Device* device);

private:
	bool CalculateNormals();
	void Shutdown();
	void ShutdownBuffers();
	bool InitializeBuffers(ID3D11Device*);
	void RenderBuffers(ID3D11DeviceContext*);
	int CountNeighbors(bool*& map, int a, int b);

	

private:
	bool m_terrainGeneratedToggle;
	int m_terrainWidth, m_terrainHeight;
	ID3D11Buffer * m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	float m_frequency, m_amplitude, m_wavelength;
	HeightMapType* m_heightMap;
	bool* m_aliveMap;

	int m_deathLimit;
	int m_birthLimit;
	int m_aliveProb;
	//arrays for our generated objects Made by directX
	std::vector<VertexPositionNormalTexture> preFabVertices;
	std::vector<uint16_t> preFabIndices;
};

