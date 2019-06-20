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

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	mTextureManager = std::make_unique<TextureManager>(md3dDevice.Get(), mCommandList.Get(), mCbvSrvUavDescriptorSize);
	mMaterialManager = std::make_unique<MaterialManager>(md3dDevice.Get());
	mMeshManager = std::make_unique<MeshManager>(md3dDevice.Get(), mCommandList.Get());
	mInstanceManager = std::make_unique<InstanceManager>(md3dDevice.Get());

	mRenderTarget = std::make_unique<RenderTarget>(md3dDevice.Get(),
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

	BuildTextures();
	BuildMaterials();
	BuildMeshes();
	BuildInstances();

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

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	if (mRenderTarget != nullptr) {
		mRenderTarget->OnResize(mClientWidth, mClientHeight);
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

	mInstanceManager->Update(gt);
	mMaterialManager->UpdateMaterialBuffer();
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
	//ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureManager->mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

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
		mBlurFilter->CopyIn(mRenderTarget->Resource());
		mBlurFilter->Execute(4);
		mBlurFilter->CopyOut(mRenderTarget->Resource());
	}

	if (mIsSobel) {
		mSobelFilter->CopyIn(mRenderTarget->Resource());
		mSobelFilter->Execute();
		mSobelFilter->CopyOut(mRenderTarget->Resource());

		mInverseFilter->CopyIn(mRenderTarget->Resource());
		mInverseFilter->Execute();
		mInverseFilter->CopyOut(mRenderTarget->Resource());
	}


	//
	// 转换回后缓冲渲染
	//

	// 改变资源状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 设置渲染目标
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mDrawQuad->CopyIn(mRenderTarget->Resource());
	mDrawQuad->Draw();

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
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void D3D12App::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3D12App::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		// 每像素对应0.25度
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
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
}

void D3D12App::OnKeyUp(WPARAM vkCode)
{
}

void D3D12App::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.FlyUp(10.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.FlyDown(10.0f * dt);
}

void D3D12App::UpdateFrameResource(const GameTimer& gt)
{
	PassConstants mainPassCB;

	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

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
	mainPassCB.EyePosW = mCamera.GetPosition3f();
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
}

void D3D12App::BuildTextures()
{
	std::vector<std::wstring> fileNames =
	{
		L"Textures/bricks.dds",
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
		L"Textures/WireFence.dds"
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
	auto bricks = std::make_unique<Material>();
	bricks->mName = "bricks";
	bricks->mDiffuseIndex = mTextureManager->mTextures["bricks"]->Index;
	bricks->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	bricks->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->mFresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks->mRoughness = 0.1f;
	mMaterialManager->AddMaterial(std::move(bricks));

	auto bricks2 = std::make_unique<Material>();
	bricks2->mName = "bricks2";
	bricks2->mDiffuseIndex = mTextureManager->mTextures["bricks2"]->Index;
	bricks2->mNormalIndex = mTextureManager->mTextures["bricks2_nmap"]->Index;
	bricks2->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks2->mFresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks2->mRoughness = 0.3f;
	mMaterialManager->AddMaterial(std::move(bricks2));

	auto stone = std::make_unique<Material>();
	stone->mName = "stone";
	stone->mDiffuseIndex = mTextureManager->mTextures["stone"]->Index;
	stone->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	stone->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->mFresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	stone->mRoughness = 0.1f;
	mMaterialManager->AddMaterial(std::move(stone));

	auto tile0 = std::make_unique<Material>();
	tile0->mName = "tile";
	tile0->mDiffuseIndex = mTextureManager->mTextures["tile"]->Index;
	tile0->mNormalIndex = mTextureManager->mTextures["tile_nmap"]->Index;
	tile0->mDiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile0->mFresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile0->mRoughness = 0.1f;
	mMaterialManager->AddMaterial(std::move(tile0));

	auto mirror0 = std::make_unique<Material>();
	mirror0->mName = "mirror";
	mirror0->mDiffuseIndex = mTextureManager->mTextures["white1x1"]->Index;
	mirror0->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	mirror0->mDiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mirror0->mFresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirror0->mRoughness = 0.1f;
	mMaterialManager->AddMaterial(std::move(mirror0));

	auto sky = std::make_unique<Material>();
	sky->mName = "sky";
	sky->mDiffuseIndex = mTextureManager->mCubeMap->Index;
	sky->mNormalIndex = -1;
	sky->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->mFresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->mRoughness = 1.0f;
	mMaterialManager->AddMaterial(std::move(sky));

	auto grass = std::make_unique<Material>();
	grass->mName = "grass";
	grass->mDiffuseIndex = mTextureManager->mTextures["grass"]->Index;
	grass->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	grass->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->mFresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->mRoughness = 0.125f;
	mMaterialManager->AddMaterial(std::move(grass));

	auto water = std::make_unique<Material>();
	water->mName = "water";
	water->mDiffuseIndex = mTextureManager->mTextures["water1"]->Index;
	water->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	water->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->mFresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	water->mRoughness = 0.0f;
	mMaterialManager->AddMaterial(std::move(water));

	auto wirefence = std::make_unique<Material>();
	wirefence->mName = "wirefence";
	wirefence->mDiffuseIndex = mTextureManager->mTextures["WireFence"]->Index;
	wirefence->mNormalIndex = mTextureManager->mTextures["default_nmap"]->Index;
	wirefence->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->mFresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wirefence->mRoughness = 0.25f;
	mMaterialManager->AddMaterial(std::move(wirefence));
}

void D3D12App::BuildMeshes()
{
	GeometryGenerator geoGen;
	mMeshManager->AddGeometry("box", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3));
	mMeshManager->AddGeometry("grid", geoGen.CreateGrid(20.0f, 30.0f, 60, 40));
	mMeshManager->AddGeometry("sphere", geoGen.CreateSphere(0.5f, 20, 20));
	mMeshManager->AddGeometry("cylinder", geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20));

	auto hill = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);
	for (auto& v : hill.Vertices) {
		v.Position.y = 0.3f * (v.Position.z * sinf(0.1f * v.Position.x) + v.Position.x * cosf(0.1f * v.Position.z));

		XMFLOAT3 n(
			-0.03f * v.Position.z * cosf(0.1f * v.Position.x) - 0.3f * cosf(0.1f * v.Position.z),
			1.0f,
			-0.3f * sinf(0.1f * v.Position.x) + 0.03f * v.Position.x * sinf(0.1f * v.Position.z));

		XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
		XMStoreFloat3(&n, unitNormal);
		v.Normal = n;
	}
	mMeshManager->AddGeometry("hill", hill);
	
	auto wave = std::make_unique<Wave>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	// 顶点
	std::vector<Vertex> vertices(wave->VertexCount());
	for (int i = 0; i < wave->VertexCount(); ++i) {
		vertices[i].Pos = wave->Position(i);
		vertices[i].Normal = wave->Normal(i);
		vertices[i].TexC.x = 0.5f + vertices[i].Pos.x / wave->Width();
		vertices[i].TexC.y = 0.5f - vertices[i].Pos.z / wave->Depth();
	}
	// 索引
	std::vector<std::uint16_t> indices(3 * wave->TriangleCount());
	assert(wave->VertexCount() < 0x0000ffff);
	int m = wave->RowCount();
	int n = wave->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i) {
		for (int j = 0; j < n - 1; ++j) {
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}
	mMeshManager->AddGeometry("wave", vertices, indices);
	wave->SetWavesVB(md3dDevice.Get());

	wave->mName = "wave";
	wave->mTranslation = XMFLOAT3(0.0f, -20.0f, 0.0f);
	wave->mMat = mMaterialManager->mMaterials["water"].get();
	XMStoreFloat4x4(&wave->mTexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wave->mGeo = mMeshManager->mGeometries["wave"].get();
	mInstanceManager->AddInstance(std::move(wave), (int)RenderLayer::Transparent);

	mMeshManager->AddGeometry("box2", geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3));
}

void D3D12App::BuildInstances()
{
	auto sky = std::make_unique<Instance>();
	sky->mName = "sky";
	sky->mScale = XMFLOAT3(5000.0f, 5000.0f, 5000.0f);
	sky->mMat = mMaterialManager->mMaterials["sky"].get();
	sky->mGeo = mMeshManager->mGeometries["sphere"].get();
	mInstanceManager->AddInstance(std::move(sky), (int)RenderLayer::Sky);

	auto box = std::make_unique<Instance>();
	box->mName = "box";
	box->mTranslation = XMFLOAT3(0.0f, 0.5f, 0.0f);
	box->mScale = XMFLOAT3(2.0f, 1.0f, 2.0f);
	box->mMat = mMaterialManager->mMaterials["bricks2"].get();
	XMStoreFloat4x4(&box->mTexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	box->mGeo = mMeshManager->mGeometries["box"].get();
	mInstanceManager->AddInstance(std::move(box), (int)RenderLayer::Opaque);

	auto globe = std::make_unique<Instance>();
	globe->mName = "globe";
	globe->mTranslation = XMFLOAT3(0.0f, 2.0f, 0.0f);
	globe->mScale = XMFLOAT3(2.0f, 2.0f, 2.0f);
	globe->mMat = mMaterialManager->mMaterials["mirror"].get();
	globe->mGeo = mMeshManager->mGeometries["sphere"].get();
	mInstanceManager->AddInstance(std::move(globe), (int)RenderLayer::Opaque);

	auto grid = std::make_unique<Instance>();
	grid->mName = "grid";
	grid->mMat = mMaterialManager->mMaterials["tile"].get();
	XMStoreFloat4x4(&grid->mTexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	grid->mGeo = mMeshManager->mGeometries["grid"].get();
	mInstanceManager->AddInstance(std::move(grid), (int)RenderLayer::Opaque);

	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
	for (int i = 0; i < 5; ++i) {
		auto leftCyl = std::make_unique<Instance>();
		leftCyl->mName = "leftCyl";
		leftCyl->mTranslation = XMFLOAT3(-5.0f, 1.5f, -10.0f + i * 5.0f);
		leftCyl->mMat = mMaterialManager->mMaterials["bricks"].get();
		XMStoreFloat4x4(&leftCyl->mTexTransform, XMMatrixScaling(1.5f, 2.0f, 1.0f));
		leftCyl->mGeo = mMeshManager->mGeometries["cylinder"].get();
		mInstanceManager->AddInstance(std::move(leftCyl), (int)RenderLayer::Opaque);

		auto rightCyl = std::make_unique<Instance>();
		rightCyl->mName = "rightCyl";
		rightCyl->mTranslation = XMFLOAT3(+5.0f, 1.5f, -10.0f + i * 5.0f);
		rightCyl->mMat = mMaterialManager->mMaterials["bricks"].get();
		XMStoreFloat4x4(&rightCyl->mTexTransform, XMMatrixScaling(1.5f, 2.0f, 1.0f));
		rightCyl->mGeo = mMeshManager->mGeometries["cylinder"].get();
		mInstanceManager->AddInstance(std::move(rightCyl), (int)RenderLayer::Opaque);

		auto leftSphere = std::make_unique<Instance>();
		leftSphere->mName = "leftSphere";
		leftSphere->mTranslation = XMFLOAT3(-5.0f, 3.5f, -10.0f + i * 5.0f);
		leftSphere->mMat = mMaterialManager->mMaterials["stone"].get();
		leftSphere->mGeo = mMeshManager->mGeometries["sphere"].get();
		mInstanceManager->AddInstance(std::move(leftSphere), (int)RenderLayer::Opaque);

		auto rightSphere = std::make_unique<Instance>();
		rightSphere->mName = "rightSphere";
		rightSphere->mTranslation = XMFLOAT3(+5.0f, 3.5f, -10.0f + i * 5.0f);
		rightSphere->mMat = mMaterialManager->mMaterials["stone"].get();
		rightSphere->mGeo = mMeshManager->mGeometries["sphere"].get();
		mInstanceManager->AddInstance(std::move(rightSphere), (int)RenderLayer::Opaque);
	}

	auto hill = std::make_unique<Hill>();
	hill->mName = "hill";
	hill->mTranslation = XMFLOAT3(0.0f, -20.0f, 0.0f);
	hill->mMat = mMaterialManager->mMaterials["grass"].get();
	XMStoreFloat4x4(&hill->mTexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	hill->mGeo = mMeshManager->mGeometries["hill"].get();
	mInstanceManager->AddInstance(std::move(hill), (int)RenderLayer::Opaque);

	auto box2 = std::make_unique<Instance>();
	box2->mName = "box2";
	box2->mTranslation = XMFLOAT3(3.0f, -18.0f, -9.0f);
	box2->mMat = mMaterialManager->mMaterials["wirefence"].get();
	box2->mGeo = mMeshManager->mGeometries["box2"].get();
	mInstanceManager->AddInstance(std::move(box2), (int)RenderLayer::AlphaTested);
}

void D3D12App::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texCubeMap;
	texCubeMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextureManager->mMaxNumTextures, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	slotRootParameter[0].InitAsConstantBufferView(0); // 常量缓冲ObjectConstants
	slotRootParameter[1].InitAsConstantBufferView(1); // 常量缓冲PassConstants
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // 结构化缓冲MaterialData
	slotRootParameter[3].InitAsDescriptorTable(1, &texCubeMap, D3D12_SHADER_VISIBILITY_PIXEL); // 天空球
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