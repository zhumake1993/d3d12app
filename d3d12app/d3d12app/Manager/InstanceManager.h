#pragma once

#include "Instance.h"

using namespace DirectX;

class InstanceManager
{
public:
	InstanceManager(ID3D12Device* device);
	~InstanceManager();

	void SetMeshManager(std::shared_ptr<MeshManager> meshManager);
	void SetMaterialManager(std::shared_ptr<MaterialManager> materialManager);
	void SetCamera(std::shared_ptr<Camera> camera);

	void AddInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const std::string& matName, const XMFLOAT4X4& texTransform,
		const std::string& meshName, const int randerLayer);

	void UpdateInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const std::string& matName, const XMFLOAT4X4& texTransform,
		const std::string& meshName, const int randerLayer);

	void UploadInstanceData(std::shared_ptr<Camera> camera);

	void Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer);

	bool Pick(FXMVECTOR rayOriginW, FXMVECTOR rayDirW);

private:
	//

public:
	std::unordered_map<std::string, std::unique_ptr<Instance>> mInstanceLayers[(int)RenderLayer::Count];

private:
	ID3D12Device* mDevice;
	std::shared_ptr<MeshManager> mMeshManager;
	std::shared_ptr<MaterialManager> mMaterialManager;
	std::shared_ptr<Camera> mCamera;
};