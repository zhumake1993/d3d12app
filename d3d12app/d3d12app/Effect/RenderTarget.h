#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class RenderTarget
{
public:
	RenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
		
	RenderTarget(const RenderTarget& rhs)=delete;
	RenderTarget& operator=(const RenderTarget& rhs)=delete;
	~RenderTarget()=default;

	ID3D12Resource* Resource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv();

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildResources();
	void BuildDescriptors();

private:
	ID3D12Device* md3dDevice = nullptr;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	ComPtr<ID3D12Resource> mRenderTargetTex = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;
};

 