#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/InstanceManager.h"

using Microsoft::WRL::ComPtr;

class DepthComplexityUseBlend
{
public:
	DepthComplexityUseBlend(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList);

	DepthComplexityUseBlend(const DepthComplexityUseBlend& rhs) = delete;
	DepthComplexityUseBlend& operator=(const DepthComplexityUseBlend& rhs) = delete;
	~DepthComplexityUseBlend() = default;

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void Draw(std::unique_ptr<InstanceManager>& instanceManager);

private:
	void BuildRootSignature();
	void BuildShader();
	void BuildPSOs();

private:
	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3DBlob> mVS = nullptr;
	ComPtr<ID3DBlob> mPS = nullptr;

	ComPtr<ID3D12PipelineState> mShowDepthComplexityUseBlendPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;
};