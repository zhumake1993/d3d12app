#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"
#include "../Common/UploadBuffer.h"
#include "MeshManager.h"

using namespace DirectX;

struct InstanceData
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InverseTransposeWorld = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex;
	UINT InstancePad0;
	UINT InstancePad1;
	UINT InstancePad2;
};

class Instance
{
	friend class InstanceManager;

public:
	Instance(ID3D12Device* device);
	virtual ~Instance();

	std::shared_ptr<Mesh> GetMesh();

	void CalculateBoundingBox();

	void AddInstanceData(const std::string& gameObjectName, const XMFLOAT4X4& world,
		const UINT& matIndex, const XMFLOAT4X4& texTransform);

	void UpdateInstanceData(std::shared_ptr<Camera> camera);

	void Draw(ID3D12GraphicsCommandList* cmdList);

private:
	//

public:
	//

private:

	std::string mMeshName;

	std::shared_ptr<Mesh> mMesh = nullptr;

	UINT mInstanceDataCapacity = 100;
	UINT mInstanceCount = 0;
	std::vector<std::unique_ptr<UploadBuffer<InstanceData>>> mFrameResources; // Ö¡×ÊÔ´vector

	UINT mVisibleCount = 0;

	BoundingBox mBounds;

	std::unordered_map<std::string, InstanceData> mInstances;
};