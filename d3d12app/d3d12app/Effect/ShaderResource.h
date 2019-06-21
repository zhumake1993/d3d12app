#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class ShaderResource
{
public:
	ShaderResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
		
	ShaderResource(const ShaderResource& rhs)=delete;
	ShaderResource& operator=(const ShaderResource& rhs)=delete;
	~ShaderResource()=default;

	ID3D12Resource* Resource();

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildResources();

private:
	ID3D12Device* md3dDevice = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	ComPtr<ID3D12Resource> mShaderResource = nullptr;
};

 