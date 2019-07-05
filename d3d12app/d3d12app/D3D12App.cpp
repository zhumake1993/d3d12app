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

	// 重置指令列表，为初始化指令做准备
	ThrowIfFailed(gCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mMainFrameResource = std::make_unique<MainFrameResource>(gD3D12Device.Get());
	gPassCB->Initialize(gD3D12Device.Get(), 1, false);

	gCamera->SetPosition(0.0f, 2.0f, -15.0f);

	BuildManagers();
	BuildRenders();
	BuildFilters();
	BuildTextures();
	BuildMaterials();
	BuildMeshes();
	BuildGameObjects();

	// 执行初始化指令
	ThrowIfFailed(gCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { gCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 等待初始化完成
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

	// 扫描帧资源环形数组
	gCurrFrameResourceIndex = (gCurrFrameResourceIndex + 1) % gNumFrameResources;

	// 判断GPU是否完成处理引用当前帧资源的指令
	// 如果没有，需等待GPU
	if (mMainFrameResource->GetCurrFence() != 0 && mFence->GetCompletedValue() < mMainFrameResource->GetCurrFence())
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mMainFrameResource->GetCurrFence(), eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// 移动光
	mLightRotationAngle += 0.1f * gTimer.DeltaTime();
	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i) {
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	// 注意，更新顺序很重要！
	gMaterialManager->UpdateMaterialData();
	gGameObjectManager->Update();
	gInputManager->Update();
	gInstanceManager->UploadInstanceData();

	mShadowMap->Update(mRotatedLightDirections[0]);

	UpdateFrameResource();
}

void D3D12App::Draw()
{
	// 获取当前的指令分配器
	auto cmdListAlloc = mMainFrameResource->GetCurrCmdListAlloc();

	//重置指令分配器以重用相关联的内存
	//必须在GPU执行完关联的指令列表后才能进行该操作
	ThrowIfFailed(cmdListAlloc->Reset());

	//重置指令列表以重用内存
	//必须在使用ExecuteCommandList将指令列表添加进指令队列后才能执行该操作
	ThrowIfFailed(gCommandList->Reset(cmdListAlloc.Get(), gPSOs["opaque"].Get()));

	//
	// 主绘制
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
		// 关闭平截头剔除
		gCamera->mFrustumCullingEnabled = false;

		mShadowMap->DrawSceneToShadowMap();

		mSsao->DrawNormalsAndDepth();
		mSsao->ComputeSsao(3);

		mCubeMap->SetShadow(mShadowMap->GetSrvDescriptorHeapPtr(), mShadowMap->Srv());
		// 这里直接使用该Ssao贴图是有问题的
		// 因为该Ssao贴图的计算是基于主摄像机，而非cubemap的6个摄像机
		// 正确的做法是基于cubemap的6个摄像机计算出6个不同的Ssao贴图
		// 这里做近似处理，直接使用该Ssao贴图
		mCubeMap->SetSsao(mSsao->GetSrvDescriptorHeapPtr(), mSsao->Srv());
		mCubeMap->DrawSceneToCubeMap();

		mMainRender->SetShadow(mShadowMap->GetSrvDescriptorHeapPtr(), mShadowMap->Srv());
		mMainRender->SetCubeMap(mCubeMap->GetSrvDescriptorHeapPtr(), mCubeMap->Srv());
		mMainRender->SetSsao(mSsao->GetSrvDescriptorHeapPtr(), mSsao->Srv());
		mMainRender->Draw(mRenderTarget->Rtv(), DepthStencilView());
	}

	//
	// 后处理
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
	// 转换回后缓冲渲染
	//

	// 改变资源状态
	gCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mDrawQuad->Draw(mRenderTarget->Resource(), CurrentBackBufferView(), DepthStencilView());

	// 改变资源状态
	gCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//关闭指令列表
	ThrowIfFailed(gCommandList->Close());

	//将指令列表添加进指令队列，供GPU执行
	ID3D12CommandList* cmdsLists[] = { gCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//交换前后缓冲
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 标记指令到达该fence点
	mMainFrameResource->GetCurrFence() = ++mCurrentFence;

	// 指令同步
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
		// 每像素对应0.25度
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

	// 将NDC空间[-1,+1]^2转换至纹理空间[0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowMap->GetShadowTransform());

	XMStoreFloat4x4(&mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
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
	mSsao->UpdateSsaoConstantData(mainPassCB);
}

void D3D12App::BuildManagers()
{
	gTextureManager->Initialize();
	gMaterialManager->Initialize();
	gMeshManager->Initialize();
	gInstanceManager->Initialize();
	gGameObjectManager->Initialize();
	gInputManager->Initialize();
}

void D3D12App::BuildRenders()
{
	mMainRender = std::make_unique<MainRender>();

	mDrawQuad = std::make_unique<DrawQuad>(gClientWidth, gClientHeight, gBackBufferFormat);
	mRenderTarget = std::make_unique<RenderTarget>(gClientWidth, gClientHeight, gBackBufferFormat);
	mShaderResourceTemp = std::make_unique<ShaderResource>(gClientWidth, gClientHeight, gBackBufferFormat);

	mWireframe = std::make_unique<Wireframe>();
	mDepthComplexityUseStencil = std::make_unique<DepthComplexityUseStencil>();
	mDepthComplexityUseBlend = std::make_unique<DepthComplexityUseBlend>();

	mCubeMap = std::make_unique<CubeMap>(DXGI_FORMAT_R8G8B8A8_UNORM, gDepthStencilFormat);
	mCubeMap->BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);

	mShadowMap = std::make_unique<ShadowMap>(2048, 2048);
	// 手动设置场景的包围球
	// 通常需要迭代所有的顶点来计算包围球
	BoundingSphere sceneBounds;
	sceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	sceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
	mShadowMap->SetBoundingSphere(sceneBounds);

	mSsao = std::make_unique<Ssao>(gClientWidth, gClientHeight);
}

void D3D12App::BuildFilters()
{
	mBlurFilter = std::make_unique<BlurFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mSobelFilter = std::make_unique<SobelFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mInverseFilter = std::make_unique<InverseFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mMultiplyFilter = std::make_unique<MultiplyFilter>(gClientWidth, gClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
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
	grass.Roughness = 0.925f;
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

void D3D12App::Pick(int sx, int sy)
{
	XMFLOAT4X4 P = gCamera->GetProj4x4f();

	// 计算视空间的选取射线
	float vx = (+2.0f * sx / gClientWidth - 1.0f) / P(0, 0);
	float vy = (-2.0f * sy / gClientHeight + 1.0f) / P(1, 1);

	// 视空间的射线定义
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	// 将射线转换至世界空间
	XMMATRIX V = gCamera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	XMVECTOR rayOriginW = XMVector3TransformCoord(rayOrigin, invView);
	XMVECTOR rayDirW = XMVector3TransformNormal(rayDir, invView);

	gInstanceManager->Pick(rayOriginW, rayDirW);
}