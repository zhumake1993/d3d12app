#pragma once

#include "../Common/d3dUtil.h"

using Microsoft::WRL::ComPtr;

class DrawQuad
{
public:
	DrawQuad(ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT descriptorSize);

	DrawQuad(const DrawQuad& rhs) = delete;
	DrawQuad& operator=(const DrawQuad& rhs) = delete;
	~DrawQuad() = default;

	void OnResize(UINT newWidth, UINT newHeight);

	void SetShader(ID3DBlob* VS, ID3DBlob* PS);
	void SetPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& opaquePsoDesc);

	void CopyIn(ID3D12Resource* input);
	void Draw();
	void Draw(ID3D12Resource* input);

private:
	void BuildResources();
	void BuildRootSignature();
	void BuildDescriptors();
	void BuildPSOs();

private:
	ID3D12Device* md3dDevice = nullptr;
	ID3D12GraphicsCommandList* mCmdList = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	UINT mCbvSrvUavDescriptorSize;

	ID3DBlob* mVS = nullptr;
	ID3DBlob* mPS = nullptr;

	ComPtr<ID3D12PipelineState> mPSO;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC mOpaquePsoDesc;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	Microsoft::WRL::ComPtr<ID3D12Resource> mTex = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mTexCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mTexGpuSrv;
};