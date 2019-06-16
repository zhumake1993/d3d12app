#pragma once

#include "d3dUtil.h"

using Microsoft::WRL::ComPtr;

struct Texture
{
	std::string Name;
	std::wstring FileName;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;

	// 纹理数组中的索引
	UINT Index;
};

class TextureManager
{
public:
	TextureManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT CbvSrvUavDescriptorSize);
	~TextureManager();

	void AddTextureTex(std::wstring FileName);
	void AddTextureCube(std::wstring FileName);
	void BuildDescriptorHeaps();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvTex();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvCube();

private:
	void CreateTexture(std::unique_ptr<Texture> &tex);

public:
	const UINT mMaxNumTextures = 100;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unique_ptr<Texture> mCubeMap;

public:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCmdList;
	UINT mCbvSrvUavDescriptorSize;
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
};