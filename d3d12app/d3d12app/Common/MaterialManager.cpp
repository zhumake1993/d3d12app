#include "MaterialManager.h"

// ======================================================================================
// Material
// ======================================================================================

Material::Material()
{
}

Material::~Material()
{
}

int Material::GetIndex()
{
	return mIndex;
}

void Material::Change()
{
	mNumFramesDirty = gNumFrameResources;
}


// ======================================================================================
// MaterialManager
// ======================================================================================

MaterialManager::MaterialManager(ID3D12Device* device)
{
	mDevice = device;

	for (int i = 0; i < gNumFrameResources; ++i) {
		mFrameResources.push_back(std::make_unique<UploadBuffer<MaterialData>>(mDevice, mMaxNumMaterials, false));
	}
}

MaterialManager::~MaterialManager()
{
}

void MaterialManager::AddMaterial(const std::string& name, UINT diffuseIndex, UINT normalIndex,
	const XMFLOAT4& diffuseAlbedo, const XMFLOAT3& fresnelR0, float roughness,
	const XMFLOAT4X4& matTransform)
{
	auto mat = std::make_unique<Material>();
	mat->mName = name;
	mat->mDiffuseIndex = diffuseIndex;
	mat->mNormalIndex = normalIndex;
	mat->mDiffuseAlbedo = diffuseAlbedo;
	mat->mFresnelR0 = fresnelR0;
	mat->mRoughness = roughness;
	mat->mMatTransform = matTransform;
	mat->mIndex = (UINT)mMaterials.size();

	mMaterials[mat->mName] = std::move(mat);
}

void MaterialManager::AddMaterial(std::unique_ptr<Material> mat)
{
	mat->mIndex = (UINT)mMaterials.size();
	mMaterials[mat->mName] = std::move(mat);
}

void MaterialManager::DeleteMaterial(std::string Name)
{
	if (mMaterials.find(Name) == mMaterials.end()) {
		OutputMessageBox("Can not find the material!");
		return;
	}

	auto mat = *mMaterials[Name];
	mMaterials.erase(Name);
	for (const auto& p : mMaterials) {
		if (p.second->mIndex > mat.mIndex) {
			p.second->mIndex--;
		}
	}
}

void MaterialManager::UpdateMaterialBuffer()
{
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];

	for (auto& e : mMaterials) {
		Material* mat = e.second.get();
		if (mat->mNumFramesDirty > 0) {
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->mMatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->mDiffuseAlbedo;
			matData.FresnelR0 = mat->mFresnelR0;
			matData.Roughness = mat->mRoughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->mDiffuseIndex;
			matData.NormalMapIndex = mat->mNormalIndex;

			uploadBuffer->CopyData(mat->mIndex, matData);

			mat->mNumFramesDirty--;
		}
	}
}

ID3D12Resource* MaterialManager::CurrResource() const
{
	return mFrameResources[gCurrFrameResourceIndex]->Resource();
}
