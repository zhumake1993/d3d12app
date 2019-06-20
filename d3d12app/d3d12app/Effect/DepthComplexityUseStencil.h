#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/InstanceManager.h"

using Microsoft::WRL::ComPtr;

class DepthComplexityUseStencil
{
public:
	DepthComplexityUseStencil(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList);

	DepthComplexityUseStencil(const DepthComplexityUseStencil& rhs) = delete;
	DepthComplexityUseStencil& operator=(const DepthComplexityUseStencil& rhs) = delete;
	~DepthComplexityUseStencil() = default;

	void SetShader(ID3DBlob* VS, ID3DBlob* PS);
	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void Draw(std::unique_ptr<InstanceManager>& instanceManager);

private:
	void BuildRootSignature();
	void BuildPSOs();

private:
	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ID3DBlob* mVS = nullptr;
	ID3DBlob* mPS = nullptr;

	ComPtr<ID3D12PipelineState> mCountDepthComplexityUseStencilPSO;
	ComPtr<ID3D12PipelineState> mShowDepthComplexityUseStencilPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;
};