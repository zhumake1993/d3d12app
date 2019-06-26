#pragma once

#include "Common/D3DApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/FrameResource.h"
#include "Common/GeometryGenerator.h"
#include "Common/Camera.h"

#include "Manager/GameObjectManager.h"
#include "Manager/InstanceManager.h"
#include "Manager/TextureManager.h"
#include "Manager/MaterialManager.h"
#include "Manager/MeshManager.h"
#include "Manager/InputManager.h"

#include "Effect/RenderTarget.h"
#include "Effect/ShaderResource.h"
#include "Effect/DrawQuad.h"
#include "Effect/Wireframe.h"
#include "Effect/DepthComplexityUseStencil.h"
#include "Effect/DepthComplexityUseBlend.h"
#include "Effect/BlurFilter.h"
#include "Effect/SobelFilter.h"
#include "Effect/InverseFilter.h"
#include "Effect/MultiplyFilter.h"
#include "Effect/CubeMap.h"

#include "GameObject/Box.h"
#include "GameObject/Hill.h"
#include "GameObject/Wave.h"
#include "GameObject/Skull.h"
#include "GameObject/Globe.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct CommonResource {

};

class D3D12App : public D3DApp
{
public:
	D3D12App(HINSTANCE hInstance);
	D3D12App(const D3D12App& rhs) = delete;
	D3D12App& operator=(const D3D12App& rhs) = delete;
	~D3D12App();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;

	virtual void Draw(const GameTimer& gt)override;


	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	virtual void OnKeyDown(WPARAM vkCode)override;
	virtual void OnKeyUp(WPARAM vkCode)override;

	void OnKeyboardInput(const GameTimer& gt);

	void UpdateFrameResource(const GameTimer& gt);

	void BuildManagers();
	void BuildEffects();
	void BuildTextures();
	void BuildMaterials();
	void BuildMeshes();
	void BuildGameObjects();

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSOs();

	void Pick(int sx, int sy);

private:
	std::shared_ptr<TextureManager> mTextureManager;
	std::shared_ptr<MaterialManager> mMaterialManager;
	std::shared_ptr<MeshManager> mMeshManager;
	std::shared_ptr<InstanceManager> mInstanceManager;
	std::shared_ptr<GameObjectManager> mGameObjectManager;

	std::shared_ptr<Camera> mCamera;
	POINT mLastMousePos;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr; // 根签名
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders; // 着色器
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; // 顶点输入布局
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs; // 渲染状态对象

	std::unique_ptr<MainFrameResource> mMainFrameResource; // Main帧资源
	std::unique_ptr<FrameResource<PassConstants>> mPassCB; // passCB帧资源

	std::unique_ptr<RenderTarget> mRenderTarget = nullptr;

	std::unique_ptr<ShaderResource> mShaderResourceTemp = nullptr;

	std::unique_ptr<DrawQuad> mDrawQuad = nullptr;

	std::unique_ptr<Wireframe> mWireframe = nullptr;
	bool mIsWireframe = false;

	std::unique_ptr<DepthComplexityUseStencil> mDepthComplexityUseStencil = nullptr;
	bool mIsDepthComplexityUseStencil = false;

	std::unique_ptr<DepthComplexityUseBlend> mDepthComplexityUseBlend = nullptr;
	bool mIsDepthComplexityUseBlend = false;

	std::unique_ptr<BlurFilter> mBlurFilter;
	bool mIsBlur = false;

	std::unique_ptr<SobelFilter> mSobelFilter;
	bool mIsSobel = false;

	std::unique_ptr<InverseFilter> mInverseFilter;
	std::unique_ptr<MultiplyFilter> mMultiplyFilter;

	std::unique_ptr<CubeMap> mCubeMap;
};

