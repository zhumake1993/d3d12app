#pragma once

#include "d3dUtil.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "UploadBuffer.h"

using namespace DirectX;

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Sky,
	UI,

	DepthComplexity1,
	DepthComplexity2,
	DepthComplexity3,
	DepthComplexity4,
	DepthComplexity5,

	Count
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InverseTransposeWorld = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

class Instance
{
	friend class InstanceManager;

public:
	Instance();
	virtual ~Instance();

	XMFLOAT4X4 GetWorld();

private:
	virtual void Update(const GameTimer& gt);
	void Change();
	void Draw(ID3D12GraphicsCommandList* cmdList);
	void UpdateObjectCB();

public:
	std::string mName;

	XMFLOAT3 mTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	Material* mMat = nullptr;
	XMFLOAT4X4 mTexTransform = MathHelper::Identity4x4();

	Mesh* mGeo = nullptr;
	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

private:
	std::vector<std::unique_ptr<UploadBuffer<ObjectConstants>>> mFrameResources; // 帧资源vector
	int mNumFramesDirty = gNumFrameResources; // 指示对象数据发生变化
};

class InstanceManager
{
public:
	InstanceManager(ID3D12Device* device);
	~InstanceManager();

	void AddInstance(std::unique_ptr<Instance> instance, int randerLayer);
	void Update(const GameTimer& gt);
	void Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer);

private:

public:
	std::vector<std::unique_ptr<Instance>> mInstances;
	std::vector<Instance*> mInstanceLayer[(int)RenderLayer::Count];

private:
	ID3D12Device* mDevice;
};