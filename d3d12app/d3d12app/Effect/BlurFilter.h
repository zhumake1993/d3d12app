#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class BlurFilter
{
public:
	BlurFilter(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT descriptorSize);

	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

	void OnResize(UINT newWidth, UINT newHeight);

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void CopyIn(ID3D12Resource* input);
	void Execute(int blurCount);
	void CopyOut(ID3D12Resource* output);
	void ExcuteInOut(ID3D12Resource* input, ID3D12Resource* output, int blurCount);

private:
	void BuildResources();
	void BuildRootSignature();
	void BuildDescriptors();
	void BuildShader();
	void BuildPSOs();

	std::vector<float> CalcGaussWeights(float sigma);

private:

	const int MaxBlurRadius = 5;

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;

	ComPtr<ID3DBlob> mHorzBlurCS;
	ComPtr<ID3DBlob> mVertBlurCS;

	ComPtr<ID3D12PipelineState> mHorzBlurPSO;
	ComPtr<ID3D12PipelineState> mVertBlurPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurTex0 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurTex1 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;
};