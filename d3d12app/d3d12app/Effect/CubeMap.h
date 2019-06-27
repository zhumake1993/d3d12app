#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/FrameResource.h"
#include "../Manager/InstanceManager.h"

using Microsoft::WRL::ComPtr;

enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

class CubeMap
{
public:
	CubeMap(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		DXGI_FORMAT format, DXGI_FORMAT mDepthStencilFormat,
		UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize, UINT dsvDescriptorSize);

	CubeMap(const CubeMap& rhs) = delete;
	CubeMap& operator=(const CubeMap& rhs) = delete;
	~CubeMap() = default;

	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	ID3D12DescriptorHeap* GetSrvDescriptorHeapPtr();

	void BuildCubeFaceCamera(float x, float y, float z);

	void OnResize(UINT newWidth, UINT newHeight);

	void UpdatePassConstantsData(PassConstants& mainPassCB);

	void DrawSceneToCubeMap(std::shared_ptr<InstanceManager> instanceManager, std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>& PSOs);

private:
	void BuildResources();
	void BuildDescriptors();

private:
	const UINT mCubeMapSize = 512;

	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;
	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<ID3D12Resource> mCubeMap = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];

	ComPtr<ID3D12Resource> mCubeDepthStencilBuffer = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCubeDSV;

	Camera mCubeMapCamera[6];

	std::vector<std::unique_ptr<UploadBuffer<PassConstants>>> mFrameResources; // Ö¡×ÊÔ´vector
};