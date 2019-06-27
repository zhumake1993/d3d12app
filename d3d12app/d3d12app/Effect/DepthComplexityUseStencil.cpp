#include "DepthComplexityUseStencil.h"

DepthComplexityUseStencil::DepthComplexityUseStencil(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList)
{
	md3dDevice = device;
	mCmdList = cmdList;

	BuildRootSignature();
}

void DepthComplexityUseStencil::SetShader(ID3DBlob* VS, ID3DBlob* PS)
{
	mVS = VS;
	mPS = PS;
}

void DepthComplexityUseStencil::SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc)
{
	mOpaquePsoDesc = opaquePsoDesc;
	BuildPSOs();
}

void DepthComplexityUseStencil::Draw(std::shared_ptr<InstanceManager> instanceManager)
{
	// 计算深度复杂度
	mCmdList->SetPipelineState(mCountDepthComplexityUseStencilPSO.Get());
	instanceManager->Draw(mCmdList, (int)RenderLayer::Opaque);
	instanceManager->Draw(mCmdList, (int)RenderLayer::OpaqueDynamicReflectors);
	instanceManager->Draw(mCmdList, (int)RenderLayer::AlphaTested);
	instanceManager->Draw(mCmdList, (int)RenderLayer::Transparent);

	// 显示深度复杂度（使用模板缓冲）
	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());

	mCmdList->SetPipelineState(mShowDepthComplexityUseStencilPSO.Get());

	float hasTex[1] = { 0.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 1, hasTex, 0);
	float pos[5] = { -1.0f, 1.0f, 0.0f, 2.0f, 2.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 5, pos, 1);

	mCmdList->IASetVertexBuffers(0, 1, nullptr);
	mCmdList->IASetIndexBuffer(nullptr);
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 1
	mCmdList->OMSetStencilRef(1);
	float color1[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color1, 6);
	mCmdList->DrawInstanced(6, 1, 0, 0);

	// 2
	mCmdList->OMSetStencilRef(2);
	float color2[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color2, 6);
	mCmdList->DrawInstanced(6, 1, 0, 0);

	// 3
	mCmdList->OMSetStencilRef(3);
	float color3[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color3, 6);
	mCmdList->DrawInstanced(6, 1, 0, 0);

	// 4
	mCmdList->OMSetStencilRef(4);
	float color4[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color4, 6);
	mCmdList->DrawInstanced(6, 1, 0, 0);

	// 5
	mCmdList->OMSetStencilRef(5);
	float color5[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color5, 6);
	mCmdList->DrawInstanced(6, 1, 0, 0);
}

void DepthComplexityUseStencil::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	slotRootParameter[0].InitAsConstants(10, 0);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);

	auto staticSamplers = d3dUtil::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void DepthComplexityUseStencil::BuildPSOs()
{
	//
	// 计算深度复杂度
	//
	CD3DX12_BLEND_DESC countDepthComplexityUseStencilBlendState(D3D12_DEFAULT);
	countDepthComplexityUseStencilBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	D3D12_DEPTH_STENCIL_DESC countDepthComplexityUseStencilDSS;
	countDepthComplexityUseStencilDSS.DepthEnable = false;
	countDepthComplexityUseStencilDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	countDepthComplexityUseStencilDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	countDepthComplexityUseStencilDSS.StencilEnable = true;
	countDepthComplexityUseStencilDSS.StencilReadMask = 0xff;
	countDepthComplexityUseStencilDSS.StencilWriteMask = 0xff;

	countDepthComplexityUseStencilDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INCR;
	countDepthComplexityUseStencilDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
	countDepthComplexityUseStencilDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	countDepthComplexityUseStencilDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// 我们不绘制背面多边形，所以这些属性无所谓
	countDepthComplexityUseStencilDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	countDepthComplexityUseStencilDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	countDepthComplexityUseStencilDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	countDepthComplexityUseStencilDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC countDepthComplexityPsoDesc = mOpaquePsoDesc;
	countDepthComplexityPsoDesc.BlendState = countDepthComplexityUseStencilBlendState;
	countDepthComplexityPsoDesc.DepthStencilState = countDepthComplexityUseStencilDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&countDepthComplexityPsoDesc, IID_PPV_ARGS(&mCountDepthComplexityUseStencilPSO)));

	//
	// 显示深度复杂度
	//
	D3D12_DEPTH_STENCIL_DESC showDepthComplexityUseStencilDSS;
	showDepthComplexityUseStencilDSS.DepthEnable = false;
	showDepthComplexityUseStencilDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	showDepthComplexityUseStencilDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	showDepthComplexityUseStencilDSS.StencilEnable = true;
	showDepthComplexityUseStencilDSS.StencilReadMask = 0xff;
	showDepthComplexityUseStencilDSS.StencilWriteMask = 0xff;

	showDepthComplexityUseStencilDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseStencilDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseStencilDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseStencilDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// 我们不绘制背面多边形，所以这些属性无所谓
	showDepthComplexityUseStencilDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseStencilDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseStencilDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	showDepthComplexityUseStencilDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC showDepthComplexityPsoDesc = mOpaquePsoDesc;
	showDepthComplexityPsoDesc.pRootSignature = mRootSignature.Get();
	showDepthComplexityPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mVS->GetBufferPointer()),
		mVS->GetBufferSize()
	};
	showDepthComplexityPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mPS->GetBufferPointer()),
		mPS->GetBufferSize()
	};
	showDepthComplexityPsoDesc.DepthStencilState = showDepthComplexityUseStencilDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&showDepthComplexityPsoDesc, IID_PPV_ARGS(&mShowDepthComplexityUseStencilPSO)));
}