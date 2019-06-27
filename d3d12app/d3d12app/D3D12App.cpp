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
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera = std::make_shared<Camera>();
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);
	mCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	BuildManagers();
	BuildEffects();
	BuildTextures();
	BuildMaterials();
	BuildMeshes();
	BuildGameObjects();

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSOs();

	mMainFrameResource = std::make_unique<MainFrameResource>(md3dDevice.Get());
	mPassCB = std::make_unique<FrameResource<PassConstants>>(md3dDevice.Get(), 1, false);

	// 执行初始化指令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 等待初始化完成
	FlushCommandQueue();

	return true;
}

void D3D12App::OnResize()
{
	D3DApp::OnResize();

	if (mCamera != nullptr) {
		mCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	}

	if (mRenderTarget != nullptr) {
		mRenderTarget->OnResize(mClientWidth, mClientHeight);
	}

	if (mShaderResourceTemp != nullptr) {
		mShaderResourceTemp->OnResize(mClientWidth, mClientHeight);
	}

	if (mDrawQuad != nullptr) {
		mDrawQuad->OnResize(mClientWidth, mClientHeight);
	}

	if (mBlurFilter != nullptr) {
		mBlurFilter->OnResize(mClientWidth, mClientHeight);
	}

	if (mSobelFilter != nullptr) {
		mSobelFilter->OnResize(mClientWidth, mClientHeight);
	}

	if (mInverseFilter != nullptr) {
		mInverseFilter->OnResize(mClientWidth, mClientHeight);
	}

	if (mMultiplyFilter != nullptr) {
		mMultiplyFilter->OnResize(mClientWidth, mClientHeight);
	}
}

void D3D12App::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

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

	// 注意，更新顺序很重要！
	mMaterialManager->UpdateMaterialData();
	mGameObjectManager->Update(gt);
	mInputManager->Update(gt);
	mInstanceManager->UploadInstanceData();
	UpdateFrameResource(gt);
}

void D3D12App::Draw(const GameTimer& gt)
{
	// 获取当前的指令分配器
	auto cmdListAlloc = mMainFrameResource->GetCurrCmdListAlloc();

	//重置指令分配器以重用相关联的内存
	//必须在GPU执行完关联的指令列表后才能进行该操作
	ThrowIfFailed(cmdListAlloc->Reset());

	//重置指令列表以重用内存
	//必须在使用ExecuteCommandList将指令列表添加进指令队列后才能执行该操作
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	//设置视口和剪裁矩形。每次重置指令列表后都要设置视口和剪裁矩形
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//清空后背缓冲和深度模板缓冲
	mCommandList->ClearRenderTargetView(mRenderTarget->Rtv(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//设置渲染目标
	mCommandList->OMSetRenderTargets(1, &mRenderTarget->Rtv(), true, &DepthStencilView());




	//
	// 主绘制
	//

	// 绑定描述符堆
	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureManager->GetSrvDescriptorHeapPtr() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 设置根签名
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 绑定常量缓冲
	auto passCB = mPassCB->GetCurrResource()->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// 绑定所有材质。对于结构化缓冲，我们可以绕过堆，使用根描述符
	auto matBuffer = mMaterialManager->CurrResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// 绑定所有的纹理
	mCommandList->SetGraphicsRootDescriptorTable(4, mTextureManager->GetGpuSrvTex());

	// 绑定天空球立方体贴图
	mCommandList->SetGraphicsRootDescriptorTable(3, mTextureManager->GetGpuSrvCube());

	if (mIsWireframe) {
		mWireframe->Draw(mInstanceManager);
	} 
	else if (mIsDepthComplexityUseStencil) {
		mDepthComplexityUseStencil->Draw(mInstanceManager);
	}
	else if (mIsDepthComplexityUseBlend) {
		mDepthComplexityUseBlend->Draw(mInstanceManager);
	}
	else {
		// 绘制动态立方体贴图时要关闭平截头剔除
		mCamera->mFrustumCullingEnabled = false;

		mCubeMap->DrawSceneToCubeMap(mInstanceManager, mPSOs);

		//设置视口和剪裁矩形。每次重置指令列表后都要设置视口和剪裁矩形
		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		//清空后背缓冲和深度模板缓冲
		mCommandList->ClearRenderTargetView(mRenderTarget->Rtv(), DirectX::Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		//设置渲染目标
		mCommandList->OMSetRenderTargets(1, &mRenderTarget->Rtv(), true, &DepthStencilView());

		// 绑定常量缓冲
		auto passCB = mPassCB->GetCurrResource()->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

		// 使用动态立方体贴图绘制动态反射物体

		// 绑定动态立方体贴图的描述符堆
		ID3D12DescriptorHeap* descriptorHeapsCube[] = { mCubeMap->GetSrvDescriptorHeapPtr() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeapsCube), descriptorHeapsCube);

		mCommandList->SetGraphicsRootDescriptorTable(3, mCubeMap->Srv());

		mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::OpaqueDynamicReflectors);

		// 使用静态立方体贴图绘制动态其他物体

		// 绑定纹理的描述符堆
		ID3D12DescriptorHeap* descriptorHeapsTex[] = { mTextureManager->GetSrvDescriptorHeapPtr() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeapsTex), descriptorHeapsTex);

		mCommandList->SetGraphicsRootDescriptorTable(3, mTextureManager->GetGpuSrvCube());

		mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::Opaque);

		mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
		mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::AlphaTested);

		mCommandList->SetPipelineState(mPSOs["transparent"].Get());
		mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::Transparent);

		mCommandList->SetPipelineState(mPSOs["sky"].Get());
		mInstanceManager->Draw(mCommandList.Get(), (int)RenderLayer::Sky);
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
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 设置渲染目标
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mDrawQuad->Draw(mRenderTarget->Resource());

	// 改变资源状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//关闭指令列表
	ThrowIfFailed(mCommandList->Close());

	//将指令列表添加进指令队列，供GPU执行
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
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

		mCamera->Pitch(dy);
		mCamera->RotateY(dx);
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
		mCamera->mFrustumCullingEnabled = !mCamera->mFrustumCullingEnabled;
	}

	mInputManager->OnKeyDown(vkCode);
}

void D3D12App::OnKeyUp(WPARAM vkCode)
{
	mInputManager->OnKeyUp(vkCode);
}

void D3D12App::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera->FlyUp(10.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera->FlyDown(10.0f * dt);
}

void D3D12App::UpdateFrameResource(const GameTimer& gt)
{
	PassConstants mainPassCB;

	XMMATRIX view = mCamera->GetView();
	XMMATRIX proj = mCamera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mainPassCB.EyePosW = mCamera->GetPosition3f();
	mainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mainPassCB.NearZ = 1.0f;
	mainPassCB.FarZ = 1000.0f;
	mainPassCB.TotalTime = gt.TotalTime();
	mainPassCB.DeltaTime = gt.DeltaTime();
	mainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	mPassCB->Copy(0, mainPassCB);

	mCubeMap->UpdatePassConstantsData(mainPassCB);
}

void D3D12App::BuildManagers()
{
	mCommonResource = std::make_shared<CommonResource>();

	mTextureManager = std::make_shared<TextureManager>(md3dDevice.Get(), mCommandList.Get(), mCbvSrvUavDescriptorSize);

	mMaterialManager = std::make_shared<MaterialManager>(md3dDevice.Get());

	mMeshManager = std::make_shared<MeshManager>(md3dDevice.Get(), mCommandList.Get());

	mInstanceManager = std::make_shared<InstanceManager>(md3dDevice.Get(), mCommonResource);

	mGameObjectManager = std::make_shared<GameObjectManager>(mCommonResource);

	mInputManager = std::make_shared<InputManager>();

	mCommonResource->mTextureManager = mTextureManager;
	mCommonResource->mMaterialManager = mMaterialManager;
	mCommonResource->mMeshManager = mMeshManager;
	mCommonResource->mInstanceManager = mInstanceManager;
	mCommonResource->mGameObjectManager = mGameObjectManager;
	mCommonResource->mInputManager = mInputManager;
	mCommonResource->mCamera = mCamera;
}

void D3D12App::BuildEffects()
{
	mRenderTarget = std::make_unique<RenderTarget>(md3dDevice.Get(),
		mClientWidth, mClientHeight, mBackBufferFormat);

	mShaderResourceTemp = std::make_unique<ShaderResource>(md3dDevice.Get(),
		mClientWidth, mClientHeight, mBackBufferFormat);

	mDrawQuad = std::make_unique<DrawQuad>(md3dDevice.Get(), mCommandList.Get(),
		mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCbvSrvUavDescriptorSize);

	mWireframe = std::make_unique<Wireframe>(md3dDevice.Get(), mCommandList.Get());

	mDepthComplexityUseStencil = std::make_unique<DepthComplexityUseStencil>(md3dDevice.Get(), mCommandList.Get());

	mDepthComplexityUseBlend = std::make_unique<DepthComplexityUseBlend>(md3dDevice.Get(), mCommandList.Get());

	mBlurFilter = std::make_unique<BlurFilter>(md3dDevice.Get(), mCommandList.Get(),
		mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCbvSrvUavDescriptorSize);

	mSobelFilter = std::make_unique<SobelFilter>(md3dDevice.Get(), mCommandList.Get(),
		mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCbvSrvUavDescriptorSize);

	mInverseFilter = std::make_unique<InverseFilter>(md3dDevice.Get(), mCommandList.Get(),
		mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCbvSrvUavDescriptorSize);

	mMultiplyFilter = std::make_unique<MultiplyFilter>(md3dDevice.Get(), mCommandList.Get(),
		mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCbvSrvUavDescriptorSize);

	mCubeMap = std::make_unique<CubeMap>(md3dDevice.Get(), mCommandList.Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM, mDepthStencilFormat,
		mCbvSrvUavDescriptorSize, mRtvDescriptorSize, mDsvDescriptorSize);
	mCubeMap->BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);
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
		mTextureManager->AddTextureTex(fileName);
	}
	mTextureManager->AddTextureCube(cubeMapFileName);

	mTextureManager->BuildDescriptorHeaps();
}

void D3D12App::BuildMaterials()
{
	MaterialData bricks;
	bricks.DiffuseMapIndex = mTextureManager->GetIndex("bricks");
	bricks.NormalMapIndex = mTextureManager->GetIndex("bricks_nmap");
	bricks.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks.FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks.Roughness = 0.3f;
	mMaterialManager->AddMaterial("bricks", bricks);

	MaterialData bricks2;
	bricks2.DiffuseMapIndex = mTextureManager->GetIndex("bricks2");
	bricks2.NormalMapIndex = mTextureManager->GetIndex("bricks2_nmap");
	bricks2.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks2.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks2.Roughness = 0.3f;
	mMaterialManager->AddMaterial("bricks2", bricks2);

	MaterialData stone;
	stone.DiffuseMapIndex = mTextureManager->GetIndex("stone");
	stone.NormalMapIndex = -1;
	stone.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone.FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	stone.Roughness = 0.1f;
	mMaterialManager->AddMaterial("stone", stone);

	MaterialData tile;
	tile.DiffuseMapIndex = mTextureManager->GetIndex("tile");
	tile.NormalMapIndex = mTextureManager->GetIndex("tile_nmap");
	tile.DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile.Roughness = 0.1f;
	mMaterialManager->AddMaterial("tile", tile);

	MaterialData mirror0;
	mirror0.DiffuseMapIndex = mTextureManager->GetIndex("white1x1");
	mirror0.NormalMapIndex = mTextureManager->GetIndex("default_nmap");
	mirror0.DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mirror0.FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirror0.Roughness = 0.1f;
	mMaterialManager->AddMaterial("mirror", mirror0);

	MaterialData sky;
	sky.DiffuseMapIndex = mTextureManager->GetCubeIndex();
	sky.NormalMapIndex = -1;
	sky.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky.Roughness = 1.0f;
	mMaterialManager->AddMaterial("sky", sky);

	MaterialData grass;
	grass.DiffuseMapIndex = mTextureManager->GetIndex("grass");
	grass.NormalMapIndex = -1;
	grass.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass.FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass.Roughness = 0.125f;
	mMaterialManager->AddMaterial("grass", grass);

	MaterialData water;
	water.DiffuseMapIndex = mTextureManager->GetIndex("water1");
	water.NormalMapIndex = -1;
	water.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	water.Roughness = 0.0f;
	mMaterialManager->AddMaterial("water", water);

	MaterialData wirefence;
	wirefence.DiffuseMapIndex = mTextureManager->GetIndex("WireFence");
	wirefence.NormalMapIndex = -1;
	wirefence.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wirefence.Roughness = 0.25f;
	mMaterialManager->AddMaterial("wirefence", wirefence);

	MaterialData ice;
	ice.DiffuseMapIndex = mTextureManager->GetIndex("ice");
	ice.NormalMapIndex = -1;
	ice.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	ice.FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	ice.Roughness = 0.0f;
	mMaterialManager->AddMaterial("ice", ice);

	MaterialData skullMat;
	skullMat.DiffuseMapIndex = mTextureManager->GetIndex("white1x1");
	skullMat.NormalMapIndex = -1;
	skullMat.DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skullMat.FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	skullMat.Roughness = 0.2f;
	mMaterialManager->AddMaterial("skullMat", skullMat);
}

void D3D12App::BuildMeshes()
{
	GeometryGenerator geoGen;
	mMeshManager->AddMesh("box", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3));
	mMeshManager->AddMesh("grid", geoGen.CreateGrid(20.0f, 30.0f, 60, 40));
	mMeshManager->AddMesh("sphere", geoGen.CreateSphere(0.5f, 20, 20));
	mMeshManager->AddMesh("cylinder", geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20));
	mMeshManager->AddMesh("box2", geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3));
}

void D3D12App::BuildGameObjects()
{
	auto sky = std::make_unique<Sky>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(sky));

	auto box = std::make_unique<Box>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(box));

	auto globe = std::make_unique<Globe>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(globe));

	auto grid = std::make_unique<Grid>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(grid));

	for (int i = 0; i < 5; ++i) {
		auto leftCyl = std::make_unique<Cylinder>(mCommonResource);
		leftCyl->mGameObjectName = "leftCyl" + std::to_string(i);
		leftCyl->mTranslation = XMFLOAT3(-5.0f, 1.5f, -10.0f + i * 5.0f);
		mGameObjectManager->AddGameObject(std::move(leftCyl));

		auto rightCyl = std::make_unique<Cylinder>(mCommonResource);
		rightCyl->mGameObjectName = "rightCyl" + std::to_string(i);
		rightCyl->mTranslation = XMFLOAT3(+5.0f, 1.5f, -10.0f + i * 5.0f);
		mGameObjectManager->AddGameObject(std::move(rightCyl));

		auto leftSphere = std::make_unique<Sphere>(mCommonResource);
		leftSphere->mGameObjectName = "leftSphere" + std::to_string(i);
		leftSphere->mTranslation = XMFLOAT3(-5.0f, 3.5f, -10.0f + i * 5.0f);
		mGameObjectManager->AddGameObject(std::move(leftSphere));

		auto rightSphere = std::make_unique<Sphere>(mCommonResource);
		rightSphere->mGameObjectName = "rightSphere" + std::to_string(i);
		rightSphere->mTranslation = XMFLOAT3(+5.0f, 3.5f, -10.0f + i * 5.0f);
		mGameObjectManager->AddGameObject(std::move(rightSphere));
	}

	auto hill = std::make_unique<Hill>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(hill));

	auto wave = std::make_unique<Wave>(mCommonResource);
	wave->SetWavesVB(md3dDevice.Get());
	mGameObjectManager->AddGameObject(std::move(wave));

	auto wirefenceBox = std::make_unique<WirefenceBox>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(wirefenceBox));

	auto skull = std::make_unique<Skull>(mCommonResource);
	mGameObjectManager->AddGameObject(std::move(skull));
}

void D3D12App::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texCubeMap;
	texCubeMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextureManager->GetMaxNumTextures(), 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	slotRootParameter[0].InitAsShaderResourceView(0, 1); // 结构化缓冲InstanceData
	slotRootParameter[1].InitAsConstantBufferView(1); // 常量缓冲PassConstants
	slotRootParameter[2].InitAsShaderResourceView(1, 1); // 结构化缓冲MaterialData
	slotRootParameter[3].InitAsDescriptorTable(1, &texCubeMap, D3D12_SHADER_VISIBILITY_PIXEL); // 立方体贴图
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // 纹理

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

void D3D12App::BuildShadersAndInputLayout()
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

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["UIVS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["UIPS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_1");

	mDrawQuad->SetShader(mShaders["UIVS"].Get(), mShaders["UIPS"].Get());
	mDepthComplexityUseStencil->SetShader(mShaders["UIVS"].Get(), mShaders["UIPS"].Get());

	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void D3D12App::BuildPSOs()
{
	//
	// 不透明物体的PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// 透明物体
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
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// alpha测试物体
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	mDrawQuad->SetPSODesc(opaquePsoDesc);
	mWireframe->SetPSODesc(opaquePsoDesc);
	mDepthComplexityUseStencil->SetPSODesc(opaquePsoDesc);
	mDepthComplexityUseBlend->SetPSODesc(opaquePsoDesc);
	mBlurFilter->SetPSODesc(opaquePsoDesc);
	mSobelFilter->SetPSODesc(opaquePsoDesc);
	mInverseFilter->SetPSODesc(opaquePsoDesc);
	mMultiplyFilter->SetPSODesc(opaquePsoDesc);

	//
	// 天空球
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// 摄像机在天空球内部，因此关闭背部剔除
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// 将深度函数从LESS改为LESS_EQUAL
	// 否则，如果深度缓冲被清空为1，那么深度值z = 1的天空球将不会通过深度测试
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = mRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
}

void D3D12App::Pick(int sx, int sy)
{
	XMFLOAT4X4 P = mCamera->GetProj4x4f();

	// 计算视空间的选取射线
	float vx = (+2.0f * sx / mClientWidth - 1.0f) / P(0, 0);
	float vy = (-2.0f * sy / mClientHeight + 1.0f) / P(1, 1);

	// 视空间的射线定义
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	// 将射线转换至世界空间
	XMMATRIX V = mCamera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	XMVECTOR rayOriginW = XMVector3TransformCoord(rayOrigin, invView);
	XMVECTOR rayDirW = XMVector3TransformNormal(rayDir, invView);

	mInstanceManager->Pick(rayOriginW, rayDirW);
}