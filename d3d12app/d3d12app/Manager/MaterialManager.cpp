#include "MaterialManager.h"

MaterialManager::MaterialManager(ID3D12Device* device)
{
	mDevice = device;

	for (int i = 0; i < gNumFrameResources; ++i) {
		mFrameResources.push_back(std::make_unique<UploadBuffer<MaterialData>>(device, mMaterialDataCapacity, false));
	}
}

MaterialManager::~MaterialManager()
{
}

UINT MaterialManager::GetIndex(const std::string& name)
{
	return mIndices[name];
}

void MaterialManager::AddMaterial(const std::string& name, UINT diffuseMapIndex, UINT normalMapIndex,
	const XMFLOAT4& diffuseAlbedo, const XMFLOAT3& fresnelR0, float roughness,
	const XMFLOAT4X4& matTransform)
{
	MaterialData mat;
	mat.DiffuseAlbedo = diffuseAlbedo;
	mat.FresnelR0 = fresnelR0;
	mat.Roughness = roughness;
	mat.MatTransform = matTransform;
	mat.DiffuseMapIndex = diffuseMapIndex;
	mat.NormalMapIndex = normalMapIndex;

	AddMaterial(name, mat);
}

void MaterialManager::AddMaterial(const std::string& name, const MaterialData& mat)
{
	if (mMaterials.find(name) != mMaterials.end()) {
		OutputMessageBox("Material already exists!");
		return;
	}

	if (MaterialCount == mMaterialDataCapacity) {
		// Ӧ�ý������ݲ���
		// ��ʱ��ʵ��
		OutputMessageBox("Can not add new material data!");
		return;
	}

	mMaterials[name] = mat;
	mIndices[name] = MaterialCount;
	mNumFramesDirties[name] = gNumFrameResources;

	++MaterialCount;
}

void MaterialManager::DeleteMaterial(std::string name)
{
	if (mMaterials.find(name) == mMaterials.end()) {
		OutputMessageBox("Can not find the material!");
		return;
	}

	--MaterialCount;

	// Ӧ�ý������ݲ���
	// ��ʱ��ʵ��

	mMaterials.erase(name);

	for (auto& p : mIndices) {
		if (p.second > mIndices[name]) {
			--p.second;
			mNumFramesDirties[p.first] = gNumFrameResources;
		}
	}
	mIndices.erase(name);

	mNumFramesDirties.erase(name);
}

void MaterialManager::MaterialDataChange(const std::string& name)
{
	mNumFramesDirties[name] = gNumFrameResources;
}

void MaterialManager::UpdateMaterialData()
{
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];

	for (auto& p : mMaterials) {
		if (mNumFramesDirties[p.first] > 0) {

			XMMATRIX matTransform = XMLoadFloat4x4(&p.second.MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = p.second.DiffuseAlbedo;
			matData.FresnelR0 = p.second.FresnelR0;
			matData.Roughness = p.second.Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = p.second.DiffuseMapIndex;
			matData.NormalMapIndex = p.second.NormalMapIndex;

			uploadBuffer->CopyData(mIndices[p.first], matData);

			--mNumFramesDirties[p.first];
		}
	}
}

ID3D12Resource* MaterialManager::CurrResource() const
{
	return mFrameResources[gCurrFrameResourceIndex]->Resource();
}