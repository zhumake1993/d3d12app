#pragma once

#include "Instance.h"
#include "Manager.h"

using namespace DirectX;

class InstanceManager :
	public Manager
{
public:
	InstanceManager(ID3D12Device* device, std::shared_ptr<CommonResource> commonResource);
	~InstanceManager();

	void AddInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const std::string& matName, const XMFLOAT4X4& texTransform,
		const std::string& meshName, const int randerLayer);

	void UpdateInstance(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const std::string& matName, const XMFLOAT4X4& texTransform,
		const std::string& meshName, const int randerLayer);

	void UploadInstanceData();

	void Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer);

	bool Pick(FXMVECTOR rayOriginW, FXMVECTOR rayDirW);

private:
	std::shared_ptr<MeshManager> GetMeshManager();
	std::shared_ptr<MaterialManager> GetMaterialManager();
	std::shared_ptr<Camera> GetCamera();

public:
	std::unordered_map<std::string, std::unique_ptr<Instance>> mInstanceLayers[(int)RenderLayer::Count];

private:
	ID3D12Device* mDevice;
	std::shared_ptr<CommonResource> mCommonResource;
};