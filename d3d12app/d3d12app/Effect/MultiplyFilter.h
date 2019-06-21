#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class MultiplyFilter
{
public:
	MultiplyFilter(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT descriptorSize);

	MultiplyFilter(const MultiplyFilter& rhs) = delete;
	MultiplyFilter& operator=(const MultiplyFilter& rhs) = delete;
	~MultiplyFilter() = default;

	void OnResize(UINT newWidth, UINT newHeight);

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void CopyIn0(ID3D12Resource* input);
	void CopyIn1(ID3D12Resource* input);
	void Execute();
	void CopyOut(ID3D12Resource* output);
	void ExcuteInOut(ID3D12Resource* input0, ID3D12Resource* input1, ID3D12Resource* output);

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

	ComPtr<ID3DBlob> mMultiplyCS;

	ComPtr<ID3D12PipelineState> mPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInput0 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mInput0CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mInput0GpuSrv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInput1 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mInput1CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mInput1GpuSrv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mOutput = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputGpuUav;
};