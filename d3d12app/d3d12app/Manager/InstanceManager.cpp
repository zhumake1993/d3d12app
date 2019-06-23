#include "InstanceManager.h"

InstanceManager::InstanceManager(ID3D12Device* device)
{
	mDevice = device;
}

InstanceManager::~InstanceManager()
{
}

void InstanceManager::SetMeshManager(std::shared_ptr<MeshManager> meshManager)
{
	mMeshManager = meshManager;
}

void InstanceManager::SetMaterialManager(std::shared_ptr<MaterialManager> materialManager)
{
	mMaterialManager = materialManager;
}

void InstanceManager::AddInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
	const std::string& matName, const XMFLOAT4X4& texTransform,
	const std::string& meshName, const int randerLayer)
{
	auto& instanceMap = mInstanceLayers[randerLayer];

	if (instanceMap.find(meshName) != instanceMap.end()) {
		// 该meshName已存在

		instanceMap[meshName]->AddInstanceData(gameObjectName, world, mMaterialManager->GetIndex(matName), texTransform);
	} else {
		// 该meshName不存在

		if (mMeshManager->mGeometries.find(meshName) == mMeshManager->mGeometries.end()) {
			OutputMessageBox("Can not find the mesh!");
			return;
		}

		auto instance = std::make_unique<Instance>(mDevice);
		instance->mMeshName = meshName;
		instance->mMesh = mMeshManager->mGeometries[meshName];
		instance->CalculateBoundingBox();

		instance->AddInstanceData(gameObjectName, world, mMaterialManager->GetIndex(matName), texTransform);

		instanceMap[meshName] = std::move(instance);
	}
}

void InstanceManager::InstanceDataChange(const std::string& gameObjectName, const std::string& meshName, const int randerLayer)
{
	auto& instanceMap = mInstanceLayers[randerLayer];
	instanceMap[meshName]->InstanceDataChange(gameObjectName);
}

void InstanceManager::UpdateInstanceData()
{
	for (auto &layer : mInstanceLayers) {
		for (auto& p : layer) {
			p.second->UpdateInstanceData();
		}
	}
}

void InstanceManager::Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer)
{
	for (auto& p : mInstanceLayers[randerLayer]) {
		p.second->Draw(cmdList);
	}
}
