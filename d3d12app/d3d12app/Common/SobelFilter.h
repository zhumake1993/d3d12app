#pragma once

#include "d3dUtil.h"

using Microsoft::WRL::ComPtr;

class SobelFilter
{
public:
	SobelFilter(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT descriptorSize);
		
	SobelFilter(const SobelFilter& rhs)=delete;
	SobelFilter& operator=(const SobelFilter& rhs)=delete;
	~SobelFilter()=default;

	ID3D12RootSignature* GetRootSignature();
	CD3DX12_GPU_DESCRIPTOR_HANDLE OutputSrv();
	ID3D12Resource* OutputResource();

	void OnResize(UINT newWidth, UINT newHeight);

	void Execute(ID3D12PipelineState* pso, ID3D12Resource* input);

private:
	void BuildResource();
	void BuildRootSignature();
	void BuildDescriptors();

private:

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuUav;

	Microsoft::WRL::ComPtr<ID3D12Resource> mOutput = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhInputCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhInputGpuSrv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInput = nullptr;
};

 