#pragma once
#pragma once
#ifndef _REFLECTIONSHADERCLASS_H_
#define _REFLECTIONSHADERCLASS_H_

#include <directxmath.h>
#include "DeviceResources.h"
#include <SimpleMath.h>
#include <d3d11.h>
#include <Model.h>


using namespace DirectX;

class ReflectionShaderClass {

public:
	ReflectionShaderClass();
	ReflectionShaderClass(const ReflectionShaderClass&);
	~ReflectionShaderClass();


	bool ReflectionShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, DirectX::SimpleMath::Matrix* worldMatrix,
		DirectX::SimpleMath::Matrix* viewMatrix, DirectX::SimpleMath::Matrix* projectionMatrix, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2,
		ID3D11ShaderResourceView* reflectionTexture, DirectX::SimpleMath::Matrix* reflectionMatrix, XMFLOAT4 color);

private:
	struct MatrixBufferType {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
	struct ReflectionBufferType
	{
		DirectX::XMMATRIX reflectionMatrix;
		XMFLOAT4 color;
	};



public:
	bool InitializeShader(ID3D11Device*, WCHAR*, WCHAR*);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);
	bool ReflectionShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext, DirectX::SimpleMath::Matrix* worldMatrix, DirectX::SimpleMath::Matrix* viewMatrix,
		DirectX::SimpleMath::Matrix* projectionMatrix, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2,  ID3D11ShaderResourceView* reflectionTexture, DirectX::SimpleMath::Matrix* reflectionMatrix, XMFLOAT4 color);
	void EnableReflectionShader(ID3D11DeviceContext*);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	 m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader >	m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11Buffer* m_matrixBuffer;
	ID3D11SamplerState* m_sampleState;
	//This is the new buffer for the reflection view matrix.
	ID3D11Buffer* m_reflectionBuffer;
};


#endif