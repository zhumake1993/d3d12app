#pragma once

#include "Instance.h"
#include "MaterialManager.h"

using namespace DirectX;

class InstanceManager
{
public:
	InstanceManager(ID3D12Device* device);
	~InstanceManager();

	void SetMeshManager(std::shared_ptr<MeshManager> meshManager);
	void SetMaterialManager(std::shared_ptr<MaterialManager> materialManager);

	void AddInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const std::string& matName, const XMFLOAT4X4& texTransform,
		const std::string& meshName, const int randerLayer);

	void InstanceDataChange(const std::string& gameObjectName, const std::string& meshName, const int randerLayer);

	void UpdateInstanceData();

	void Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer);

private:

public:
	std::unordered_map<std::string, std::unique_ptr<Instance>> mInstanceLayers[(int)RenderLayer::Count];

private:
	ID3D12Device* mDevice;
	std::shared_ptr<MeshManager> mMeshManager;
	std::shared_ptr<MaterialManager> mMaterialManager;
};