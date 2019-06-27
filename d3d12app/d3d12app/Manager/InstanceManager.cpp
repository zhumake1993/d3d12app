#include "InstanceManager.h"

InstanceManager::InstanceManager(ID3D12Device* device, std::shared_ptr<CommonResource> commonResource)
{
	mDevice = device;
	mCommonResource = commonResource;
}

InstanceManager::~InstanceManager()
{
}

void InstanceManager::AddInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
	const std::string& matName, const XMFLOAT4X4& texTransform,
	const std::string& meshName, const int randerLayer)
{
	auto& instanceMap = mInstanceLayers[randerLayer];

	if (instanceMap.find(meshName) != instanceMap.end()) {
		// 该meshName已存在

		instanceMap[meshName]->AddInstanceData(gameObjectName, world, GetMaterialManager()->GetIndex(matName), texTransform);
	} else {
		// 该meshName不存在

		if (GetMeshManager()->mMeshes.find(meshName) == GetMeshManager()->mMeshes.end()) {
			OutputMessageBox("Can not find the mesh!");
			return;
		}

		auto instance = std::make_unique<Instance>(mDevice, mCommonResource);
		instance->mMeshName = meshName;
		instance->mMesh = GetMeshManager()->mMeshes[meshName];
		instance->CalculateBoundingBox();
		instance->GetCamera() = GetCamera();

		instance->AddInstanceData(gameObjectName, world, GetMaterialManager()->GetIndex(matName), texTransform);

		instanceMap[meshName] = std::move(instance);
	}
}

void InstanceManager::UpdateInstance(const std::string& gameObjectName, const XMFLOAT4X4& world, 
	const std::string& matName, const XMFLOAT4X4& texTransform, 
	const std::string& meshName, const int randerLayer)
{
	auto& instanceMap = mInstanceLayers[randerLayer];
	instanceMap[meshName]->UpdateInstanceData(gameObjectName, world, GetMaterialManager()->GetIndex(matName), texTransform);
}

void InstanceManager::UploadInstanceData()
{
	for (auto &layer : mInstanceLayers) {
		for (auto& p : layer) {
			p.second->UploadInstanceData();
		}
	}
}

void InstanceManager::Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer)
{
	for (auto& p : mInstanceLayers[randerLayer]) {
		p.second->Draw(cmdList);
	}
}

bool InstanceManager::Pick(FXMVECTOR rayOriginW, FXMVECTOR rayDirW)
{
	bool result = false;

	std::string nameResult;
	float tminResult = MathHelper::Infinity;
	XMVECTOR pointResult;

	for (int i = 0; i < (int)RenderLayer::Count;i++) {

		// 排除天空球
		if (i == (int)RenderLayer::Sky)
			continue;

		for (auto& p : mInstanceLayers[i]) {

			std::string name;
			float tmin;
			XMVECTOR point;

			if (p.second->Pick(rayOriginW, rayDirW, name, tmin, point)) {
				if (tmin < tminResult) {
					result = true;

					nameResult = name;
					tminResult = tmin;
					pointResult = point;
				}
			}
		}
	}

	if (result) {
		XMFLOAT3 pp;
		XMStoreFloat3(&pp, pointResult);
		OutputDebug(nameResult);
		OutputDebug(std::to_string(tminResult));
		OutputDebug(std::to_string(pp.x) + ',' + std::to_string(pp.y) + ',' + std::to_string(pp.z));
	}

	return result;
}

std::shared_ptr<MeshManager> InstanceManager::GetMeshManager()
{
	return std::static_pointer_cast<MeshManager>(mCommonResource->mMeshManager);
}

std::shared_ptr<MaterialManager> InstanceManager::GetMaterialManager()
{
	return std::static_pointer_cast<MaterialManager>(mCommonResource->mMaterialManager);
}

std::shared_ptr<Camera> InstanceManager::GetCamera()
{
	return std::static_pointer_cast<Camera>(mCommonResource->mCamera);
}
