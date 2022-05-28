
#include "pch.h"
#include "Reflection.h"

using namespace DirectX;

ReflectionShaderClass::ReflectionShaderClass()
{

}



ReflectionShaderClass::~ReflectionShaderClass()
{
}

bool ReflectionShaderClass::InitializeShader(ID3D11Device* device, WCHAR* vsFilename, WCHAR* psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
	unsigned int numElements;

	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC reflectionBufferDesc;

	auto vertexShaderBuffer = DX::ReadData(vsFilename);//load vertex file
	result = device->CreateVertexShader(vertexShaderBuffer.data(), vertexShaderBuffer.size(), NULL, &m_vertexShader);
	if (result != S_OK)
	{
		//if loading failed.  
		return false;
	}

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	D3D11_INPUT_ELEMENT_DESC polygonLayout2[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//{ "COLOR", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	//,
	//,
	//{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout2) / sizeof(polygonLayout2[0]);

	// Create the vertex input layout.
	device->CreateInputLayout(polygonLayout2, numElements, vertexShaderBuffer.data(), vertexShaderBuffer.size(), &m_layout);


	auto pixelShaderBuffer = DX::ReadData(psFilename);
	result = device->CreatePixelShader(pixelShaderBuffer.data(), pixelShaderBuffer.size(), NULL, &m_pixelShader);
	if (result != S_OK)
	{
		//if loading failed. 
		return false;
	}

	// Setup the description of the matrix dynamic constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;


	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	device->CreateSamplerState(&samplerDesc, &m_sampleState);


	//Here we setup the new reflection matrix dynamic constant buffer.

	// Setup the description of the reflection dynamic constant buffer that is in the vertex shader.
	reflectionBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	reflectionBufferDesc.ByteWidth = sizeof(ReflectionBufferType);
	reflectionBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	reflectionBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	reflectionBufferDesc.MiscFlags = 0;
	reflectionBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	device->CreateBuffer(&reflectionBufferDesc, NULL, &m_reflectionBuffer);

	return true;
}

bool ReflectionShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext, DirectX::SimpleMath::Matrix* worldMatrix, DirectX::SimpleMath::Matrix* viewMatrix,
	DirectX::SimpleMath::Matrix* projectionMatrix,  ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2, ID3D11ShaderResourceView* reflectionTexture, DirectX::SimpleMath::Matrix* reflectionMatrix, XMFLOAT4 color)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	unsigned int bufferNumber;
	ReflectionBufferType* dataPtr2;

	DirectX::SimpleMath::Matrix  tworld, tview, tproj, treflec;
	tworld = worldMatrix->Transpose();
	tview = viewMatrix->Transpose();
	tproj = projectionMatrix->Transpose();
	treflec = reflectionMatrix->Transpose();


	// Lock the matrix constant buffer so it can be written to.
	result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the matrix constant buffer.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// Copy the matrices into the matrix constant buffer.
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;

	// Unlock the matrix constant buffer.
	deviceContext->Unmap(m_matrixBuffer, 0);

	// Set the position of the matrix constant buffer in the vertex shader.
	bufferNumber = 0;

	// Now set the matrix constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
	//Lock the reflection bufferand copy the reflection matrix into it.After that unlock itand set it in the vertex shader.

	// Lock the reflection constant buffer so it can be written to.
	result = deviceContext->Map(m_reflectionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the matrix constant buffer.
	dataPtr2 = (ReflectionBufferType*)mappedResource.pData;

	// Copy the matrix into the reflection constant buffer.
	dataPtr2->reflectionMatrix = treflec;
	dataPtr2->color = color;
	// Unlock the reflection constant buffer.
	deviceContext->Unmap(m_reflectionBuffer, 0);

	// Set the position of the reflection constant buffer in the vertex shader.
	bufferNumber = 1;

	// Now set the reflection constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_reflectionBuffer);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);
	//Set the reflection texture as the second texture inside the pixel shader.

	deviceContext->PSSetShaderResources(1, 1, &texture2);

	// Set the reflection texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(2, 1, &reflectionTexture);

	return true;
}


void ReflectionShaderClass::EnableReflectionShader(ID3D11DeviceContext* deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(m_layout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(m_vertexShader.Get(), 0, 0);
	deviceContext->PSSetShader(m_pixelShader.Get(), 0, 0);

	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &m_sampleState);
	//// Render the triangle.
	//deviceContext->DrawIndexed(indexCount, 0, 0);

	//return;
}
