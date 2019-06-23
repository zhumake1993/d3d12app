#include "Wireframe.h"

Wireframe::Wireframe(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList)
{
	md3dDevice = device;
	mCmdList = cmdList;
}

void Wireframe::SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc)
{
	mOpaquePsoDesc = opaquePsoDesc;
	BuildPSOs();
}

void Wireframe::Draw(std::shared_ptr<InstanceManager> instanceManager)
{
	mCmdList->SetPipelineState(mWireframePSO.Get());
	instanceManager->Draw(mCmdList, (int)RenderLayer::Opaque);
	instanceManager->Draw(mCmdList, (int)RenderLayer::AlphaTested);
	instanceManager->Draw(mCmdList, (int)RenderLayer::Transparent);
}

void Wireframe::BuildPSOs()
{
	// 不透明线框物体的PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = mOpaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mWireframePSO)));
}