#include "D3D12App.h"

D3D12App::D3D12App(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

D3D12App::~D3D12App()
{
}

bool D3D12App::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// ����ָ���б�Ϊ��ʼ��ָ����׼��
	ThrowIfFailed(gCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mMainFrameResource = std::make_unique<MainFrameResource>(gD3D12Device.Get());
	gPassCB->Initialize(gD3D12Device.Get(), 1, false);

	gCamera->SetPosition(0.0f, 2.0f, -15.0f);

	BuildRootSignature();
	BuildShaders();

	BuildManagers();
	BuildRenders();
	BuildFilters();
	BuildTextures();
	BuildMaterials();
	BuildMeshes();
	BuildGameObjects();

	BuildPSOs();

	// ִ�г�ʼ��ָ��
	ThrowIfFailed(gCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { gCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ȴ���ʼ�����
	FlushCommandQueue();

	return true;
}

void D3D12App::OnResize()
{
	D3DApp::OnResize();

	gCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void D3D12App::Update()
{
	OnKeyboardInput();

	// ɨ��֡��Դ��������
	gCurrFrameResourceIndex = (gCurrFrameResourceIndex + 1) % gNumFrameResources;

	// �ж�GPU�Ƿ���ɴ������õ�ǰ֡��Դ��ָ��
	// ���û�У���ȴ�GPU
	if (mMainFrameResource->GetCurrFence() != 0 && mFence->GetCompletedValue() < mMainFrameResource->GetCurrFence())
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mMainFrameResource->GetCurrFence(), eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// �ƶ���
	mLightRotationAngle += 0.1f * gTimer.DeltaTime();
	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i) {
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	// ע�⣬����˳�����Ҫ��
	gMaterialManager->UpdateMaterialData();
	gGameObjectManager->Update();
	gInputManager->Update();
	gInstanceManager->UploadInstanceData();

	//
	mShadowMap->Update(mRotatedLightDirections[0]);

	UpdateFrameResource();
}

void D3D12App::Draw()
{
	// ��ȡ��ǰ��ָ�������
	auto cmdListAlloc = mMainFrameResource->GetCurrCmdListAlloc();

	//����ָ���������������������ڴ�
	//������GPUִ���������ָ���б����ܽ��иò���
	ThrowIfFailed(cmdListAlloc->Reset());

	//����ָ���б��������ڴ�
	//������ʹ��ExecuteCommandList��ָ���б���ӽ�ָ����к����ִ�иò���
	ThrowIfFailed(gCommandList->Reset(cmdListAlloc.Get(), gPSOs["opaque"].Get()));

	//
	// ������
	//

	if (mIsWireframe) {
		mWireframe->Draw(mRenderTarget->Rtv(), DepthStencilView());
	} 
	else if (mIsDepthComplexityUseStencil) {
		mDepthComplexityUseStencil->Draw(mRenderTarget->Rtv(), DepthStencilView());
	}
	else if (mIsDepthComplexityUseBlend) {
		mDepthComplexityUseBlend->Draw(mRenderTarget->Rtv(), DepthStencilView());
		mInverseFilter->ExcuteInOut(mRenderTarget->Resource(), mRenderTarget->Resource());
	}
	else {
		// ����������
		ID3D12DescriptorHeap* descriptorHeaps[] = { gTextureManager->GetSrvDescriptorHeapPtr() };
		gCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		// ���ø�ǩ��
		gCommandList->SetGraphicsRootSignature(gRootSignatures["main"].Get());

		// �󶨳�������
		auto passCB = gPassCB->GetCurrResource()->Resource();
		gCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		// �����в��ʡ����ڽṹ�����壬���ǿ����ƹ��ѣ�ʹ�ø�������
		auto matBuffer = gMaterialManager->CurrResource();
		gCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

		// �����е�����
		gCommandList->SetGraphicsRootDescriptorTable(5, gTextureManager->GetGpuSrvTex());

		// ���������������ͼ
		gCommandList->SetGraphicsRootDescriptorTable(3, gTextureManager->GetGpuSrvCube());




		// ������ӰʱҪ�ر�ƽ��ͷ�޳�
		gCamera->mFrustumCullingEnabled = false;

		mShadowMap->DrawSceneToShadowMap();

		//�����ӿںͼ��þ��Ρ�ÿ������ָ���б��Ҫ�����ӿںͼ��þ���
		gCommandList->RSSetViewports(1, &gScreenViewport);
		gCommandList->RSSetScissorRects(1, &gScissorRect);

		//��պ󱳻�������ģ�建��
		gCommandList->ClearRenderTargetView(mRenderTarget->Rtv(), DirectX::Colors::Black, 0, nullptr);
		gCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		//������ȾĿ��
		gCommandList->OMSetRenderTargets(1, &mRenderTarget->Rtv(), true, &DepthStencilView());

		// �󶨳�������
		passCB = gPassCB->GetCurrResource()->Resource();
		gCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		// ���������������ͼ
		gCommandList->SetGraphicsRootDescriptorTable(3, gTextureManager->GetGpuSrvCube());

		// ����Ӱ��ͼ
		ID3D12DescriptorHeap* descriptorHeapsCube[] = { mShadowMap->GetSrvDescriptorHeapPtr() };
		gCommandList->SetDescriptorHeaps(_countof(descriptorHeapsCube), descriptorHeapsCube);
		gCommandList->SetGraphicsRootDescriptorTable(4, mShadowMap->Srv());

		//// ���ƶ�̬��������ͼʱҪ�ر�ƽ��ͷ�޳�
		//mCamera->mFrustumCullingEnabled = false;

		//mCubeMap->DrawSceneToCubeMap(mInstanceManager, mPSOs);

		////�����ӿںͼ��þ��Ρ�ÿ������ָ���б��Ҫ�����ӿںͼ��þ���
		//mCommandList->RSSetViewports(1, &mScreenViewport);
		//mCommandList->RSSetScissorRects(1, &mScissorRect);

		////��պ󱳻�������ģ�建��
		//mCommandList->ClearRenderTargetView(mRenderTarget->Rtv(), DirectX::Colors::LightSteelBlue, 0, nullptr);
		//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		////������ȾĿ��
		//mCommandList->OMSetRenderTargets(1, &mRenderTarget->Rtv(), true, &DepthStencilView());

		//// �󶨳�������
		//auto passCB = mPassCB->GetCurrResource()->Resource();
		//mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		//// ʹ�ö�̬��������ͼ���ƶ�̬��������

		//// �󶨶�̬��������ͼ����������
		//ID3D12DescriptorHeap* descriptorHeapsCube[] = { mCubeMap->GetSrvDescriptorHeapPtr() };
		//mCommandList->SetDescriptorHeaps(_countof(descriptorHeapsCube), descriptorHeapsCube);

		//mCommandList->SetGraphicsRootDescriptorTable(3, mCubeMap->Srv());

		//mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		//mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::OpaqueDynamicReflectors);

		//// ʹ�þ�̬��������ͼ���ƶ�̬��������

		//// ���������������
		//ID3D12DescriptorHeap* descriptorHeapsTex[] = { mTextureManager->GetSrvDescriptorHeapPtr() };
		//mCommandList->SetDescriptorHeaps(_countof(descriptorHeapsTex), descriptorHeapsTex);

		//mCommandList->SetGraphicsRootDescriptorTable(3, mTextureManager->GetGpuSrvCube());

		gCommandList->SetPipelineState(gPSOs["opaque"].Get());
		gInstanceManager->Draw(gCommandList.Get(), (int)RenderLayer::Opaque);

		gCommandList->SetPipelineState(gPSOs["alphaTested"].Get());
		gInstanceManager->Draw(gCommandList.Get(), (int)RenderLayer::AlphaTested);

		gCommandList->SetPipelineState(gPSOs["transparent"].Get());
		gInstanceManager->Draw(gCommandList.Get(), (int)RenderLayer::Transparent);

		gCommandList->SetPipelineState(gPSOs["sky"].Get());
		gInstanceManager->Draw(gCommandList.Get(), (int)RenderLayer::Sky);
	}

	//
	// ����
	//

	if (mIsBlur) {
		mBlurFilter->ExcuteInOut(mRenderTarget->Resource(), mRenderTarget->Resource(), 4);
	}

	if (mIsSobel) {
		mSobelFilter->ExcuteInOut(mRenderTarget->Resource(), mShaderResourceTemp->Resource());
		mInverseFilter->ExcuteInOut(mShaderResourceTemp->Resource(), mShaderResourceTemp->Resource());
		mMultiplyFilter->ExcuteInOut(mRenderTarget->Resource(), mShaderResourceTemp->Resource(), mRenderTarget->Resource());
	}

	//
	// ת���غ󻺳���Ⱦ
	//

	// �ı���Դ״̬
	gCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mDrawQuad->Draw(mRenderTarget->Resource(), CurrentBackBufferView(), DepthStencilView());

	// �ı���Դ״̬
	gCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//�ر�ָ���б�
	ThrowIfFailed(gCommandList->Close());

	//��ָ���б���ӽ�ָ����У���GPUִ��
	ID3D12CommandList* cmdsLists[] = { gCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//����ǰ�󻺳�
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// ���ָ����fence��
	mMainFrameResource->GetCurrFence() = ++mCurrentFence;

	// ָ��ͬ��
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void D3D12App::OnMouseDown(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0) {
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mhMainWnd);
	} else if ((btnState & MK_LBUTTON) != 0) {
		Pick(x, y);
	}
}

void D3D12App::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3D12App::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0) {
		// ÿ���ض�Ӧ0.25��
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		gCamera->Pitch(dy);
		gCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void D3D12App::OnKeyDown(WPARAM vkCode)
{
	if (vkCode == '1') {
		mIsWireframe = !mIsWireframe;
		mIsDepthComplexityUseStencil = false;
		mIsDepthComplexityUseBlend = false;
	}

	if (vkCode == '2') {
		mIsDepthComplexityUseStencil = !mIsDepthComplexityUseStencil;
		mIsWireframe = false;
		mIsDepthComplexityUseBlend = false;
	}

	if (vkCode == '3') {
		mIsDepthComplexityUseBlend = !mIsDepthComplexityUseBlend;
		mIsWireframe = false;
		mIsDepthComplexityUseStencil = false;
	}

	if (vkCode == '4') {
		mIsBlur = !mIsBlur;
	}

	if (vkCode == '5') {
		mIsSobel = !mIsSobel;
	}

	if (vkCode == '6') {
		gCamera->mFrustumCullingEnabled = !gCamera->mFrustumCullingEnabled;
	}

	gInputManager->OnKeyDown(vkCode);
}

void D3D12App::OnKeyUp(WPARAM vkCode)
{
	gInputManager->OnKeyUp(vkCode);
}

void D3D12App::OnKeyboardInput()
{
	const float dt = gTimer.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		gCamera->Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		gCamera->Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		gCamera->Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		gCamera->Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		gCamera->FlyUp(10.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		gCamera->FlyDown(10.0f * dt);
}

void D3D12App::UpdateFrameResource()
{
	PassConstants mainPassCB;

	XMMATRIX view = gCamera->GetView();
	XMMATRIX proj = gCamera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowMap->GetShadowTransform());

	XMStoreFloat4x4(&mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));

	mainPassCB.EyePosW = gCamera->GetPosition3f();
	mainPassCB.RenderTargetSize = XMFLOAT2((float)gClientWidth, (float)gClientHeight);
	mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / gClientWidth, 1.0f / gClientHeight);
	mainPassCB.NearZ = 1.0f;
	mainPassCB.FarZ = 1000.0f;
	mainPassCB.TotalTime = gTimer.TotalTime();
	mainPassCB.DeltaTime = gTimer.DeltaTime();
	mainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
	mainPassCB.Lights[0].Strength = { 0.9f, 0.8f, 0.7f };
	mainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
	mainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
	mainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	gPassCB->Copy(0, mainPassCB);

	mCubeMap->UpdatePassConstantsData(mainPassCB);
}

void D3D12App::BuildManagers()
{
	gTextureManager->Initialize(gD3D12Device.Get(), gCommandList.Get(), gCbvSrvUavDescriptorSize);
	gMaterialManager->Initialize(gD3D12Device.Get());
	gMeshManager->Initialize(gD3D12Device.Get(), gCommandList.Get());
	gInstanceManager->Initialize(gD3D12Device.Get());
	gGameObjectManager->Initialize();
	gInputManager->Initialize();
}

void D3D12App::BuildRenders()
{
	mDrawQuad = std::make_unique<DrawQuad>(gClientWidth, gClientHeight, gBackBufferFormat);
	mRenderTarget = std::make_unique<RenderTarget>(gClientWidth, gClientHeight, gBackBufferFormat);
	mShaderResourceTemp = std::make_unique<ShaderResource>(gClientWidth, gClientHeight, gBackBufferFormat);

	mWireframe = std::make_unique<Wireframe>();
	mDepthComplexityUseStencil = std::make_unique<DepthComplexityUseStencil>();
	mDepthComplexityUseBlend = std::make_unique<DepthComplexityUseBlend>();
}

void D3D12App::BuildFilters()
{
	mBlurFilter = std::make_unique<BlurFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mSobelFilter = std::make_unique<SobelFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mInverseFilter = std::make_unique<InverseFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mMultiplyFilter = std::make_unique<MultiplyFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

	mCubeMap = std::make_unique<CubeMap>(gD3D12Device.Get(), gCommandList.Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM, gDepthStencilFormat,
		gCbvSrvUavDescriptorSize, gRtvDescriptorSize, gDsvDescriptorSize);
	mCubeMap->BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);

	mShadowMap = std::make_unique<ShadowMap>(gD3D12Device.Get(), gCommandList.Get(), 2048, 2048);
	// �ֶ����ó����İ�Χ��
	// ͨ����Ҫ�������еĶ����������Χ��
	BoundingSphere sceneBounds;
	sceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	sceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
	mShadowMap->SetBoundingSphere(sceneBounds);

}

void D3D12App::BuildTextures()
{
	std::vector<std::wstring> fileNames =
	{
		L"Textures/bricks.dds",
		L"Textures/bricks_nmap.dds",
		L"Textures/bricks2.dds",
		L"Textures/bricks2_nmap.dds",
		L"Textures/stone.dds",
		L"Textures/tile.dds",
		L"Textures/tile_nmap.dds",
		L"Textures/white1x1.dds",
		L"Textures/default_nmap.dds",
		L"Textures/grass.dds",
		L"Textures/water1.dds",
		L"Textures/WoodCrate01.dds",
		L"Textures/WireFence.dds",
		L"Textures/ice.dds"
	};

	std::wstring cubeMapFileName = L"Textures/snowcube1024.dds";

	for (auto fileName : fileNames) {
		gTextureManager->AddTextureTex(fileName);
	}
	gTextureManager->AddTextureCube(cubeMapFileName);

	gTextureManager->BuildDescriptorHeaps();
}

void D3D12App::BuildMaterials()
{
	MaterialData bricks;
	bricks.DiffuseMapIndex = gTextureManager->GetIndex("bricks");
	bricks.NormalMapIndex = gTextureManager->GetIndex("bricks_nmap");
	bricks.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks.FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks.Roughness = 0.3f;
	gMaterialManager->AddMaterial("bricks", bricks);

	MaterialData bricks2;
	bricks2.DiffuseMapIndex = gTextureManager->GetIndex("bricks2");
	bricks2.NormalMapIndex = gTextureManager->GetIndex("bricks2_nmap");
	bricks2.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks2.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks2.Roughness = 0.3f;
	gMaterialManager->AddMaterial("bricks2", bricks2);

	MaterialData stone;
	stone.DiffuseMapIndex = gTextureManager->GetIndex("stone");
	stone.NormalMapIndex = -1;
	stone.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone.FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	stone.Roughness = 0.1f;
	gMaterialManager->AddMaterial("stone", stone);

	MaterialData tile;
	tile.DiffuseMapIndex = gTextureManager->GetIndex("tile");
	tile.NormalMapIndex = gTextureManager->GetIndex("tile_nmap");
	tile.DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile.Roughness = 0.1f;
	gMaterialManager->AddMaterial("tile", tile);

	MaterialData mirror0;
	mirror0.DiffuseMapIndex = gTextureManager->GetIndex("white1x1");
	mirror0.NormalMapIndex = gTextureManager->GetIndex("default_nmap");
	mirror0.DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mirror0.FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirror0.Roughness = 0.1f;
	gMaterialManager->AddMaterial("mirror", mirror0);

	MaterialData sky;
	sky.DiffuseMapIndex = gTextureManager->GetCubeIndex();
	sky.NormalMapIndex = -1;
	sky.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky.Roughness = 1.0f;
	gMaterialManager->AddMaterial("sky", sky);

	MaterialData grass;
	grass.DiffuseMapIndex = gTextureManager->GetIndex("grass");
	grass.NormalMapIndex = -1;
	grass.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass.FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass.Roughness = 0.125f;
	gMaterialManager->AddMaterial("grass", grass);

	MaterialData water;
	water.DiffuseMapIndex = gTextureManager->GetIndex("water1");
	water.NormalMapIndex = -1;
	water.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	water.Roughness = 0.0f;
	gMaterialManager->AddMaterial("water", water);

	MaterialData wirefence;
	wirefence.DiffuseMapIndex = gTextureManager->GetIndex("WireFence");
	wirefence.NormalMapIndex = -1;
	wirefence.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wirefence.Roughness = 0.25f;
	gMaterialManager->AddMaterial("wirefence", wirefence);

	MaterialData ice;
	ice.DiffuseMapIndex = gTextureManager->GetIndex("ice");
	ice.NormalMapIndex = -1;
	ice.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	ice.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	ice.Roughness = 0.0f;
	gMaterialManager->AddMaterial("ice", ice);

	MaterialData skullMat;
	skullMat.DiffuseMapIndex = gTextureManager->GetIndex("white1x1");
	skullMat.NormalMapIndex = -1;
	skullMat.DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skullMat.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	skullMat.Roughness = 0.2f;
	gMaterialManager->AddMaterial("skullMat", skullMat);
}

void D3D12App::BuildMeshes()
{
	GeometryGenerator geoGen;
	gMeshManager->AddMesh("box", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3));
	gMeshManager->AddMesh("grid", geoGen.CreateGrid(20.0f, 30.0f, 60, 40));
	gMeshManager->AddMesh("sphere", geoGen.CreateSphere(0.5f, 20, 20));
	gMeshManager->AddMesh("cylinder", geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20));
	gMeshManager->AddMesh("box2", geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3));
}

void D3D12App::BuildGameObjects()
{
	auto sky = std::make_unique<Sky>();
	gGameObjectManager->AddGameObject(std::move(sky));

	auto box = std::make_unique<Box>();
	gGameObjectManager->AddGameObject(std::move(box));

	auto globe = std::make_unique<Globe>();
	gGameObjectManager->AddGameObject(std::move(globe));

	auto grid = std::make_unique<Grid>();
	gGameObjectManager->AddGameObject(std::move(grid));

	for (int i = 0; i < 5; ++i) {
		auto leftCyl = std::make_unique<Cylinder>();
		leftCyl->mGameObjectName = "leftCyl" + std::to_string(i);
		leftCyl->mTranslation = XMFLOAT3(-5.0f, 1.5f, -10.0f + i * 5.0f);
		gGameObjectManager->AddGameObject(std::move(leftCyl));

		auto rightCyl = std::make_unique<Cylinder>();
		rightCyl->mGameObjectName = "rightCyl" + std::to_string(i);
		rightCyl->mTranslation = XMFLOAT3(+5.0f, 1.5f, -10.0f + i * 5.0f);
		gGameObjectManager->AddGameObject(std::move(rightCyl));

		auto leftSphere = std::make_unique<Sphere>();
		leftSphere->mGameObjectName = "leftSphere" + std::to_string(i);
		leftSphere->mTranslation = XMFLOAT3(-5.0f, 3.5f, -10.0f + i * 5.0f);
		gGameObjectManager->AddGameObject(std::move(leftSphere));

		auto rightSphere = std::make_unique<Sphere>();
		rightSphere->mGameObjectName = "rightSphere" + std::to_string(i);
		rightSphere->mTranslation = XMFLOAT3(+5.0f, 3.5f, -10.0f + i * 5.0f);
		gGameObjectManager->AddGameObject(std::move(rightSphere));
	}

	auto hill = std::make_unique<Hill>();
	gGameObjectManager->AddGameObject(std::move(hill));

	auto wave = std::make_unique<Wave>();
	wave->SetWavesVB(gD3D12Device.Get());
	gGameObjectManager->AddGameObject(std::move(wave));

	auto wirefenceBox = std::make_unique<WirefenceBox>();
	gGameObjectManager->AddGameObject(std::move(wirefenceBox));

	auto skull = std::make_unique<Skull>();
	gGameObjectManager->AddGameObject(std::move(skull));
}

void D3D12App::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texCubeMap;
	texCubeMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texShadowMap;
	texShadowMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gTextureManager->GetMaxNumTextures(), 2, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	slotRootParameter[0].InitAsShaderResourceView(0, 1); // �ṹ������InstanceData
	slotRootParameter[1].InitAsConstantBufferView(1); // ��������PassConstants
	slotRootParameter[2].InitAsShaderResourceView(1, 1); // �ṹ������MaterialData
	slotRootParameter[3].InitAsDescriptorTable(1, &texCubeMap, D3D12_SHADER_VISIBILITY_PIXEL); // ��������ͼ
	slotRootParameter[4].InitAsDescriptorTable(1, &texShadowMap, D3D12_SHADER_VISIBILITY_PIXEL); // ��Ӱ��ͼ
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // ����

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

	ThrowIfFailed(gD3D12Device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(gRootSignatures["main"].GetAddressOf())));
}

void D3D12App::BuildShaders()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	gShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	gShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	gShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	gShaders["UIVS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_1");
	gShaders["UIPS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_1");

	gShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	gShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
}

void D3D12App::BuildPSOs()
{
	//
	// ��͸�������PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { gInputLayout.data(), (UINT)gInputLayout.size() };
	opaquePsoDesc.pRootSignature = gRootSignatures["main"].Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(gShaders["standardVS"]->GetBufferPointer()),
		gShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(gShaders["opaquePS"]->GetBufferPointer()),
		gShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = gBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = g4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = g4xMsaaState ? (g4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = gDepthStencilFormat;
	ThrowIfFailed(gD3D12Device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&gPSOs["opaque"])));

	//
	// ͸������
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(gD3D12Device->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&gPSOs["transparent"])));

	//
	// alpha��������
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(gShaders["alphaTestedPS"]->GetBufferPointer()),
		gShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(gD3D12Device->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&gPSOs["alphaTested"])));

	mShadowMap->SetPSODesc(opaquePsoDesc);

	//
	// �����
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// �������������ڲ�����˹رձ����޳�
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// ����Ⱥ�����LESS��ΪLESS_EQUAL
	// ���������Ȼ��屻���Ϊ1����ô���ֵz = 1������򽫲���ͨ����Ȳ���
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = gRootSignatures["main"].Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(gShaders["skyVS"]->GetBufferPointer()),
		gShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(gShaders["skyPS"]->GetBufferPointer()),
		gShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(gD3D12Device->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&gPSOs["sky"])));
}

void D3D12App::Pick(int sx, int sy)
{
	XMFLOAT4X4 P = gCamera->GetProj4x4f();

	// �����ӿռ��ѡȡ����
	float vx = (+2.0f * sx / gClientWidth - 1.0f) / P(0, 0);
	float vy = (-2.0f * sy / gClientHeight + 1.0f) / P(1, 1);

	// �ӿռ�����߶���
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	// ������ת��������ռ�
	XMMATRIX V = gCamera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	XMVECTOR rayOriginW = XMVector3TransformCoord(rayOrigin, invView);
	XMVECTOR rayDirW = XMVector3TransformNormal(rayDir, invView);

	gInstanceManager->Pick(rayOriginW, rayDirW);
}