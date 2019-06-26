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

void InstanceManager::SetCamera(std::shared_ptr<Camera> camera)
{
	mCamera = camera;
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

		if (mMeshManager->mMeshes.find(meshName) == mMeshManager->mMeshes.end()) {
			OutputMessageBox("Can not find the mesh!");
			return;
		}

		auto instance = std::make_unique<Instance>(mDevice);
		instance->mMeshName = meshName;
		instance->mMesh = mMeshManager->mMeshes[meshName];
		instance->CalculateBoundingBox();
		instance->mCamera = mCamera;

		instance->AddInstanceData(gameObjectName, world, mMaterialManager->GetIndex(matName), texTransform);

		instanceMap[meshName] = std::move(instance);
	}
}

void InstanceManager::UpdateInstance(const std::string& gameObjectName, const XMFLOAT4X4& world, 
	const std::string& matName, const XMFLOAT4X4& texTransform, 
	const std::string& meshName, const int randerLayer)
{
	auto& instanceMap = mInstanceLayers[randerLayer];
	instanceMap[meshName]->UpdateInstanceData(gameObjectName, world, mMaterialManager->GetIndex(matName), texTransform);
}

void InstanceManager::UploadInstanceData(std::shared_ptr<Camera> camera)
{
	for (auto &layer : mInstanceLayers) {
		for (auto& p : layer) {
			p.second->UploadInstanceData(camera);
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
