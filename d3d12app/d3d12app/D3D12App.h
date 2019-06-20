#pragma once

#include "Common/D3DApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/FrameResource.h"
#include "Common/GeometryGenerator.h"
#include "Common/Camera.h"
#include "Common/TextureManager.h"
#include "Common/MaterialManager.h"
#include "Common/MeshManager.h"
#include "Common/InstanceManager.h"

#include "Effect/RenderTarget.h"
#include "Effect/DrawQuad.h"
#include "Effect/Wireframe.h"
#include "Effect/DepthComplexityUseStencil.h"
#include "Effect/DepthComplexityUseBlend.h"
#include "Effect/BlurFilter.h"
#include "Effect/SobelFilter.h"
#include "Effect/InverseFilter.h"

#include "GameObject/Hill.h"
#include "GameObject/Wave.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

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

	void BuildTextures();
	void BuildMaterials();
	void BuildMeshes();
	void BuildInstances();

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSOs();

private:
	std::unique_ptr<TextureManager> mTextureManager;
	std::unique_ptr<MaterialManager> mMaterialManager;
	std::unique_ptr<MeshManager> mMeshManager;
	std::unique_ptr<InstanceManager> mInstanceManager;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr; // ��ǩ��
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders; // ��ɫ��
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; // �������벼��
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs; // ��Ⱦ״̬����

	std::unique_ptr<MainFrameResource> mMainFrameResource; // Main֡��Դ
	std::unique_ptr<FrameResource<PassConstants>> mPassCB; // passCB֡��Դ

	Camera mCamera;

	POINT mLastMousePos;

	std::unique_ptr<RenderTarget> mRenderTarget = nullptr;

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
};

