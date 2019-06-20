#include "DrawQuad.h"

DrawQuad::DrawQuad(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	UINT width, UINT height,
	DXGI_FORMAT format, UINT descriptorSize)
{
	md3dDevice = device;
	mCmdList = cmdList;

	mWidth = width;
	mHeight = height;
	mFormat = format;
	mCbvSrvUavDescriptorSize = descriptorSize;

	BuildResources();
	BuildRootSignature();
	BuildDescriptors();
}

void DrawQuad::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight)) {
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResources();
		BuildDescriptors();
	}
}

void DrawQuad::SetShader(ID3DBlob* VS, ID3DBlob* PS)
{
	mVS = VS;
	mPS = PS;
}

void DrawQuad::SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc)
{
	mOpaquePsoDesc = opaquePsoDesc;
	BuildPSOs();
}

void DrawQuad::CopyIn(ID3D12Resource* input)
{
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTex.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

	mCmdList->CopyResource(mTex.Get(), input);

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mTex.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void DrawQuad::Draw()
{
	// 注意：由于使用了另一个描述符堆，需要重新调用SetDescriptorHeaps
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	mCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());

	mCmdList->SetPipelineState(mPSO.Get());

	float hasTex[1] = { 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 1, hasTex, 0);
	float pos[5] = { -1.0f, 1.0f, 0.0f, 2.0f, 2.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 5, pos, 1);
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCmdList->SetGraphicsRoot32BitConstants(0, 4, color, 6);

	mCmdList->SetGraphicsRootDescriptorTable(1, mTexGpuSrv);

	// 使用着色器中的SV_VertexID来构建顶点
	mCmdList->IASetVertexBuffers(0, 1, nullptr);
	mCmdList->IASetIndexBuffer(nullptr);
	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->DrawInstanced(6, 1, 0, 0);
}

void DrawQuad::BuildResources()
{
	// 注意：压缩格式不兼容UAV
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mTex)));
}

void DrawQuad::BuildRootSignature()
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

void DrawQuad::BuildDescriptors()
{
	// 创建SRV堆
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mCbvSrvUavDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor(mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// 保留描述符的引用
	mTexCpuSrv = hCpuDescriptor;
	mTexGpuSrv = hGpuDescriptor;

	// 创建描述符
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	md3dDevice->CreateShaderResourceView(mTex.Get(), &srvDesc, mTexCpuSrv);
}

void DrawQuad::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC fullScreenQuadPSODesc = mOpaquePsoDesc;
	fullScreenQuadPSODesc.pRootSignature = mRootSignature.Get();

	// 关闭深度测试
	fullScreenQuadPSODesc.DepthStencilState.DepthEnable = false;
	fullScreenQuadPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	fullScreenQuadPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	fullScreenQuadPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mVS->GetBufferPointer()),
		mVS->GetBufferSize()
	};
	fullScreenQuadPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mPS->GetBufferPointer()),
		mPS->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&fullScreenQuadPSODesc, IID_PPV_ARGS(&mPSO)));
}