#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/InstanceManager.h"

using Microsoft::WRL::ComPtr;

class Wireframe
{
public:
	Wireframe(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList);

	Wireframe(const Wireframe& rhs) = delete;
	Wireframe& operator=(const Wireframe& rhs) = delete;
	~Wireframe() = default;

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void Draw(std::unique_ptr<InstanceManager>& instanceManager);

private:
	void BuildPSOs();

private:
	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12PipelineState> mWireframePSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;
};