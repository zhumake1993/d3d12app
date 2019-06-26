#include "CubeMap.h"

CubeMap::CubeMap(ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	DXGI_FORMAT format, DXGI_FORMAT depthStencilFormat,
	UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize, UINT dsvDescriptorSize)
{
	md3dDevice = device;
	mCmdList = cmdList;

	for (int i = 0; i < gNumFrameResources; ++i) {
		mFrameResources.push_back(std::make_unique<UploadBuffer<PassConstants>>(device, 6, true));
	}

	mWidth = mCubeMapSize;
	mHeight = mCubeMapSize;
	mFormat = format;
	mDepthStencilFormat = depthStencilFormat;

	mViewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)mWidth, (int)mHeight };

	mCbvSrvUavDescriptorSize = cbvSrvUavDescriptorSize;
	mRtvDescriptorSize = rtvDescriptorSize;
	mDsvDescriptorSize = dsvDescriptorSize;

	BuildResources();
	BuildDescriptors();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE CubeMap::Srv()
{
	return mhGpuSrv;
}

ID3D12DescriptorHeap* CubeMap::GetSrvDescriptorHeapPtr()
{
	return mCbvSrvUavDescriptorHeap.Get();;
}

void CubeMap::BuildCubeFaceCamera(float x, float y, float z)
{
	// ��������ͼ�����ĵ�
	XMFLOAT3 center(x, y, z);

	// ����
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// up����
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	for (int i = 0; i < 6; ++i) {
		mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		mCubeMapCamera[i].SetLens(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}
}

void CubeMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight)) {
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResources();

		// �µ���Դ�������Ҫ�����µ�������
		BuildDescriptors();
	}
}

void CubeMap::UpdatePassConstantsData(PassConstants& mainPassCB)
{
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];

	for (int i = 0; i < 6; ++i) {
		PassConstants cubeFacePassCB = mainPassCB;

		XMMATRIX view = mCubeMapCamera[i].GetView();
		XMMATRIX proj = mCubeMapCamera[i].GetProj();

		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

		XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&cubeFacePassCB.InvViewProj, XMMatrixTranspose(invViewProj));
		cubeFacePassCB.EyePosW = mCubeMapCamera[i].GetPosition3f();
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)mCubeMapSize, (float)mCubeMapSize);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mCubeMapSize, 1.0f / mCubeMapSize);

		uploadBuffer->CopyData(i, cubeFacePassCB);
	}
}

void CubeMap::DrawSceneToCubeMap(std::shared_ptr<InstanceManager> instanceManager, std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>& PSOs)
{
	mCmdList->RSSetViewports(1, &mViewport);
	mCmdList->RSSetScissorRects(1, &mScissorRect);

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// ����ÿһ����������ͼ��
	for (int i = 0; i < 6; ++i) {
		// ��պ󻺳����Ȼ���
		mCmdList->ClearRenderTargetView(mhCpuRtv[i], Colors::LightSteelBlue, 0, nullptr);
		mCmdList->ClearDepthStencilView(mhCubeDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// ָ����ȾĿ��
		mCmdList->OMSetRenderTargets(1, &mhCpuRtv[i], true, &mhCubeDSV);

		// �󶨳�������
		auto uploadBuffer = mFrameResources[gCurrFrameResourceIndex]->Resource();
		mCmdList->SetGraphicsRootConstantBufferView(1, uploadBuffer->GetGPUVirtualAddress() + i * passCBByteSize);

		mCmdList->SetPipelineState(PSOs["opaque"].Get());
		instanceManager->Draw(mCmdList, (int)RenderLayer::Opaque);

		mCmdList->SetPipelineState(PSOs["alphaTested"].Get());
		instanceManager->Draw(mCmdList, (int)RenderLayer::AlphaTested);

		mCmdList->SetPipelineState(PSOs["transparent"].Get());
		instanceManager->Draw(mCmdList, (int)RenderLayer::Transparent);

		mCmdList->SetPipelineState(PSOs["sky"].Get());
		instanceManager->Draw(mCmdList, (int)RenderLayer::Sky);
	}

	// ����Դ״̬�Ļ�GENERIC_READ��ʹ���ܹ�����ɫ���ж�ȡ����
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void CubeMap::BuildResources()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6; // 6����
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;  // ������ȾĿ��

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&CD3DX12_CLEAR_VALUE(mFormat, DirectX::Colors::LightSteelBlue),
		IID_PPV_ARGS(&mCubeMap)));
}

void CubeMap::BuildDescriptors()
{
	// ����SRV��
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1; // ��ʱֻ��һ����̬��������ͼ
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mCbvSrvUavDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor(mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// ����������������
	mhCpuSrv = hCpuDescriptor;
	mhGpuSrv = hGpuDescriptor;

	// ����������
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	// ����������������ͼ��Դ��SRV
	md3dDevice->CreateShaderResourceView(mCubeMap.Get(), &srvDesc, mhCpuSrv);



	// ����RTV��
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 6;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	for (int i = 0; i < 6; ++i)
		mhCpuRtv[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), i, mRtvDescriptorSize);

	// ����ÿ�����������RTV
	for (int i = 0; i < 6; ++i) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY; //����2D����
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		// ��i��Ԫ��
		rtvDesc.Texture2DArray.FirstArraySlice = i;

		// ֻ��1������Ԫ��
		rtvDesc.Texture2DArray.ArraySize = 1;

		// ����RTV
		md3dDevice->CreateRenderTargetView(mCubeMap.Get(), &rtvDesc, mhCpuRtv[i]);
	}



	// ����DSV��
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mhCubeDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 0, mDsvDescriptorSize);

	// �������ģ�建�����ͼ
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mCubeMapSize;
	depthStencilDesc.Height = mCubeMapSize;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(mCubeDepthStencilBuffer.GetAddressOf())));

	// �������ģ����ͼ
	md3dDevice->CreateDepthStencilView(mCubeDepthStencilBuffer.Get(), nullptr, mhCubeDSV);
}