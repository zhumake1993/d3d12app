#pragma once

#include "d3dUtil.h"

using Microsoft::WRL::ComPtr;

class RenderTarget
{
public:
	RenderTarget(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format);
		
	RenderTarget(const RenderTarget& rhs)=delete;
	RenderTarget& operator=(const RenderTarget& rhs)=delete;
	~RenderTarget()=default;

	ID3D12Resource* Resource();
	ID3D12RootSignature* GetRootSignature();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv();

	void OnResize(UINT newWidth, UINT newHeight);

	void TestCopy(ID3D12Resource* input);
	void DrawToBackBuffer(ID3D12PipelineState* PSO);

private:
	void BuildResource();
	void BuildRootSignature();
	void BuildDescriptors();

private:

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

	ComPtr<ID3D12Resource> mOffscreenTex = nullptr;
};

 