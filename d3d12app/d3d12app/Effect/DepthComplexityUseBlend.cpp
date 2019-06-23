#include "DepthComplexityUseBlend.h"

DepthComplexityUseBlend::DepthComplexityUseBlend(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList)
{
	md3dDevice = device;
	mCmdList = cmdList;

	BuildRootSignature();
	BuildShader();
}

void DepthComplexityUseBlend::SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc)
{
	mOpaquePsoDesc = opaquePsoDesc;
	BuildPSOs();
}

void DepthComplexityUseBlend::Draw(std::shared_ptr<InstanceManager> instanceManager)
{
	mCmdList->SetPipelineState(mShowDepthComplexityUseBlendPSO.Get());
	instanceManager->Draw(mCmdList, (int)RenderLayer::Opaque);
	instanceManager->Draw(mCmdList, (int)RenderLayer::AlphaTested);
	instanceManager->Draw(mCmdList, (int)RenderLayer::Transparent);
}

void DepthComplexityUseBlend::BuildRootSignature()
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

void DepthComplexityUseBlend::BuildShader()
{
	mVS = d3dUtil::CompileShader(L"Shaders\\Grey.hlsl", nullptr, "VS", "vs_5_1");
	mPS = d3dUtil::CompileShader(L"Shaders\\Grey.hlsl", nullptr, "PS", "ps_5_1");
}

void DepthComplexityUseBlend::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC showDepthComplexityUseBlendPsoDesc = mOpaquePsoDesc;
	showDepthComplexityUseBlendPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mVS->GetBufferPointer()),
		mVS->GetBufferSize()
	};
	showDepthComplexityUseBlendPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mPS->GetBufferPointer()),
		mPS->GetBufferSize()
	};

	D3D12_RENDER_TARGET_BLEND_DESC showDepthComplexityUseBlendBlendDesc;
	showDepthComplexityUseBlendBlendDesc.BlendEnable = true;
	showDepthComplexityUseBlendBlendDesc.LogicOpEnable = false;
	showDepthComplexityUseBlendBlendDesc.SrcBlend = D3D12_BLEND_ONE;
	showDepthComplexityUseBlendBlendDesc.DestBlend = D3D12_BLEND_ONE;
	showDepthComplexityUseBlendBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	showDepthComplexityUseBlendBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	showDepthComplexityUseBlendBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	showDepthComplexityUseBlendBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	showDepthComplexityUseBlendBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	showDepthComplexityUseBlendBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_DEPTH_STENCIL_DESC showDepthComplexityUseBlendDSS;
	showDepthComplexityUseBlendDSS.DepthEnable = false;
	showDepthComplexityUseBlendDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	showDepthComplexityUseBlendDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	showDepthComplexityUseBlendDSS.StencilEnable = false;
	showDepthComplexityUseBlendDSS.StencilReadMask = 0xff;
	showDepthComplexityUseBlendDSS.StencilWriteMask = 0xff;

	showDepthComplexityUseBlendDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseBlendDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseBlendDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseBlendDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// 我们不绘制背面多边形，所以这些属性无所谓
	showDepthComplexityUseBlendDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseBlendDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	showDepthComplexityUseBlendDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	showDepthComplexityUseBlendDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	showDepthComplexityUseBlendPsoDesc.BlendState.RenderTarget[0] = showDepthComplexityUseBlendBlendDesc;
	showDepthComplexityUseBlendPsoDesc.DepthStencilState = showDepthComplexityUseBlendDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&showDepthComplexityUseBlendPsoDesc, IID_PPV_ARGS(&mShowDepthComplexityUseBlendPSO)));
}