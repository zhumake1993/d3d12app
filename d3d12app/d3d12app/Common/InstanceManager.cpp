#include "InstanceManager.h"

// ======================================================================================
// Instance
// ======================================================================================

Instance::Instance()
{
}

Instance::~Instance()
{
}

XMFLOAT4X4 Instance::GetWorld()
{
	XMMATRIX T = XMMatrixTranslation(mTranslation.x, mTranslation.y, mTranslation.z);
	XMMATRIX R = XMMatrixRotationX(mRotation.x) * XMMatrixRotationX(mRotation.y) * XMMatrixRotationX(mRotation.z);
	XMMATRIX S = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
	XMMATRIX W = S * R * T; // ע��˳��
	XMFLOAT4X4 mWorld;
	XMStoreFloat4x4(&mWorld, W);
	return mWorld;
}

void Instance::Update(const GameTimer& gt)
{
}

void Instance::Change()
{
	mNumFramesDirty = gNumFrameResources;
}

void Instance::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &mGeo->VertexBufferView);
	cmdList->IASetIndexBuffer(&mGeo->IndexBufferView);
	cmdList->IASetPrimitiveTopology(mPrimitiveType);
	cmdList->SetGraphicsRootConstantBufferView(0, mFrameResources[gCurrFrameResourceIndex]->Resource()->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(mGeo->IndexCount, 1, 0, 0, 0);
}

void Instance::UpdateObjectCB()
{
	auto &uploadBuffer = mFrameResources[gCurrFrameResourceIndex];
	if (mNumFramesDirty > 0) {
		XMMATRIX world = XMLoadFloat4x4(&GetWorld());
		XMMATRIX texTransform = XMMatrixScaling(mTexTransform.x, mTexTransform.y, mTexTransform.z);

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
		objConstants.MaterialIndex = mMat->GetIndex();

		uploadBuffer->CopyData(0, objConstants);

		mNumFramesDirty--;
	}
}

// ======================================================================================
// InstanceManager
// ======================================================================================

InstanceManager::InstanceManager(ID3D12Device* device)
{
	mDevice = device;
}

InstanceManager::~InstanceManager()
{
}

void InstanceManager::AddInstance(std::unique_ptr<Instance> instance, int randerLayer)
{
	for (int i = 0; i < gNumFrameResources; ++i) {
		instance->mFrameResources.push_back(std::make_unique<UploadBuffer<ObjectConstants>>(mDevice, 1, true));
	}

	// ע�⣺move�������ú�instanceʧЧ��֮���ܵ���instance.get()����
	// ��ˣ�move����Ҫ����get�ĺ���
	mInstanceLayer[randerLayer].push_back(instance.get());
	mInstances.push_back(std::move(instance));
}

void InstanceManager::Update(const GameTimer& gt)
{
	for (auto &ins : mInstances) {
		ins->Update(gt);
		ins->UpdateObjectCB();
	}
}

void InstanceManager::Draw(ID3D12GraphicsCommandList* cmdList, int randerLayer)
{
	for (auto ins : mInstanceLayer[randerLayer]) {
		ins->Draw(cmdList);
	}
}
