#pragma once

#include "d3dUtil.h"
#include "UploadBuffer.h"

using namespace DirectX;

struct MaterialData
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.5f;

	XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT NormalMapIndex = 0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

class Material
{
	friend class MaterialManager;

public:
	Material();
	~Material();

	int GetIndex();
	void Change();

private:
	//

public:
	std::string mName;

	UINT mDiffuseIndex = -1;
	UINT mNormalIndex = -1;
	XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 mFresnelR0 = { 0.01f, 0.01f, 0.01f };
	float mRoughness = .25f;
	XMFLOAT4X4 mMatTransform = MathHelper::Identity4x4();

private:
	int mNumFramesDirty = gNumFrameResources; // 指示对象数据发生变化
	UINT mIndex = -1;
};

class MaterialManager
{
public:
	MaterialManager(ID3D12Device* device);
	~MaterialManager();

	void AddMaterial(const std::string& name, UINT diffuseIndex, UINT normalIndex,
		const XMFLOAT4& diffuseAlbedo, const XMFLOAT3& fresnelR0, float roughness,
		const XMFLOAT4X4& mMatTransform);
	void AddMaterial(std::unique_ptr<Material> mat);
	void DeleteMaterial(std::string name);

	void UpdateMaterialBuffer();

	ID3D12Resource* CurrResource()const;

public:
	const UINT mMaxNumMaterials = 100;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

private:
	ID3D12Device* mDevice;
	std::vector<std::unique_ptr<UploadBuffer<MaterialData>>> mFrameResources; // 帧资源vector
};