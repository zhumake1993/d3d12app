#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/FrameResource.h"
#include "../Manager/InstanceManager.h"

using Microsoft::WRL::ComPtr;

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height);

	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

	void SetBoundingSphere(BoundingSphere sceneBounds);

	XMFLOAT4X4 GetShadowTransform();

	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	ID3D12DescriptorHeap* GetSrvDescriptorHeapPtr();

	void OnResize(UINT newWidth, UINT newHeight);

	void Update(XMFLOAT3 mRotatedLightDirection);

	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void DrawSceneToShadowMap(std::shared_ptr<InstanceManager> instanceManager);

private:
	void BuildResources();
	void BuildRootSignature();
	void BuildDescriptors();
	void BuildShader();
	void BuildPSOs();

private:

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;
	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;

	ComPtr<ID3DBlob> mShadowVS;
	ComPtr<ID3DBlob> mShadowPS;
	ComPtr<ID3DBlob> mShadowAlphaTestedPS;

	ComPtr<ID3D12PipelineState> mPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	BoundingSphere mSceneBounds;
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	ComPtr<ID3D12Resource> mShadowMap = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	std::vector<std::unique_ptr<UploadBuffer<PassConstants>>> mFrameResources; // Ö¡×ÊÔ´vector
};

