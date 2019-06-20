#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class InverseFilter
{
public:
	InverseFilter(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT descriptorSize);

	InverseFilter(const InverseFilter& rhs) = delete;
	InverseFilter& operator=(const InverseFilter& rhs) = delete;
	~InverseFilter() = default;

	void OnResize(UINT newWidth, UINT newHeight);

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void CopyIn(ID3D12Resource* input);
	void Execute();
	void CopyOut(ID3D12Resource* output);

private:
	void BuildResources();
	void BuildRootSignature();
	void BuildDescriptors();
	void BuildShader();
	void BuildPSOs();

private:

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;

	ComPtr<ID3DBlob> mInverseVS;
	ComPtr<ID3DBlob> mInversePS;

	ComPtr<ID3D12PipelineState> mPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInput = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mInputCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mInputGpuSrv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mOutput = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputGpuUav;
};