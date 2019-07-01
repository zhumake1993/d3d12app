#include "ShadowMap.h"

ShadowMap::ShadowMap(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	UINT width, UINT height)
{
	md3dDevice = device;
	mCmdList = cmdList;

	for (int i = 0; i < gNumFrameResources; ++i) {
		mFrameResources.push_back(std::make_unique<UploadBuffer<PassConstants>>(device, 1, true));
	}

	mWidth = width;
	mHeight = height;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResources();
	BuildRootSignature();
	BuildDescriptors();
	BuildShader();
}

void ShadowMap::SetBoundingSphere(BoundingSphere sceneBounds)
{
	mSceneBounds = sceneBounds;
}

XMFLOAT4X4 ShadowMap::GetShadowTransform()
{
	return mShadowTransform;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()
{
	return mhGpuSrv;
}

ID3D12DescriptorHeap* ShadowMap::GetSrvDescriptorHeapPtr()
{
	return mCbvSrvUavDescriptorHeap.Get();;
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight)) {
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResources();

		// 新的资源，因此需要创建新的描述符
		BuildDescriptors();
	}
}

void ShadowMap::Update(XMFLOAT3 mRotatedLightDirection)
{
	// 只有第一个光源产生阴影
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirection);
	XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	// 将包围球转换至光空间
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// 光空间中的正交平截头
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// 将NDC空间[-1,+1]^2转换至纹理空间[0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mShadowTransform, S);

	// 更新PCB
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];
	PassConstants mShadowPassCB;

	XMMATRIX view = lightView;
	XMMATRIX proj = lightProj;

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mWidth;
	UINT h = mHeight;

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	XMStoreFloat3(&mShadowPassCB.EyePosW, lightPos);
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = n;
	mShadowPassCB.FarZ = f;

	uploadBuffer->CopyData(0, mShadowPassCB);
}

void ShadowMap::SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc)
{
	mOpaquePsoDesc = opaquePsoDesc;
	BuildPSOs();
}

void ShadowMap::DrawSceneToShadowMap(std::shared_ptr<InstanceManager> instanceManager)
{
	mCmdList->RSSetViewports(1, &mViewport);
	mCmdList->RSSetScissorRects(1, &mScissorRect);

	// 转换至DEPTH_WRITE.
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 清空缓冲
	mCmdList->ClearDepthStencilView(mhCpuDsv,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 指定空渲染目标，因为只绘制深度缓冲
	// 设置空渲染目标会关闭颜色写
	// 对应的PSO必须同时指定渲染目标的数量为0
	mCmdList->OMSetRenderTargets(0, nullptr, false, &mhCpuDsv);

	// 绑定常量缓冲
	auto uploadBuffer = mFrameResources[gCurrFrameResourceIndex]->Resource();
	mCmdList->SetGraphicsRootConstantBufferView(1, uploadBuffer->GetGPUVirtualAddress());

	mCmdList->SetPipelineState(mPSO.Get());
	instanceManager->Draw(mCmdList, (int)RenderLayer::Opaque);

	//mCmdList->SetPipelineState(PSOs["alphaTested"].Get());
	//instanceManager->Draw(mCmdList, (int)RenderLayer::AlphaTested);

	//mCmdList->SetPipelineState(PSOs["transparent"].Get());
	//instanceManager->Draw(mCmdList, (int)RenderLayer::Transparent);

	// 转换回GENERIC_READ，使得能够在着色器中读取该纹理
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void ShadowMap::BuildResources()
{
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
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}

void ShadowMap::BuildRootSignature()
{

}

void ShadowMap::BuildDescriptors()
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
	mhCpuSrv = hCpuDescriptor;
	mhGpuSrv = hGpuDescriptor;

	// 创建SRV，以便在着色器程序中采样阴影贴图
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	md3dDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mhCpuSrv);



	// 创建DSV堆
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mhCpuDsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 0, mDsvDescriptorSize);

	// 创建DSV，以便渲染到阴影贴图
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mhCpuDsv);
}

void ShadowMap::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShadowVS = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShadowPS = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	mShadowAlphaTestedPS = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");
}

void ShadowMap::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = mOpaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	//smapPsoDesc.pRootSignature = mRootSignature.Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShadowVS->GetBufferPointer()),
		mShadowVS->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShadowPS->GetBufferPointer()),
		mShadowPS->GetBufferSize()
	};

	// 绘制阴影贴图不需要渲染目标
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSO)));
}