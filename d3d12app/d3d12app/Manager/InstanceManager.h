#pragma once

#include "Instance.h"
#include "MaterialManager.h"

using namespace DirectX;

class InstanceManager
{
public:
	InstanceManager();
	~InstanceManager();

	void Initialize(ID3D12Device* device);

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
	//

public:
	std::unordered_map<std::string, std::unique_ptr<Instance>> mInstanceLayers[(int)RenderLayer::Count];

private:
	ID3D12Device* mDevice;
};

extern std::unique_ptr<InstanceManager> gInstanceManager;