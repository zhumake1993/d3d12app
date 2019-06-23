#include "Instance.h"

Instance::Instance(ID3D12Device* device)
{
	for (int i = 0; i < gNumFrameResources; ++i) {
		mFrameResources.push_back(std::make_unique<UploadBuffer<InstanceData>>(device, mInstanceDataCapacity, false));
	}
}

Instance::~Instance()
{
}

void Instance::CalculateBoundingBox()
{
	//
	//
	//
}

void Instance::AddInstanceData(const std::string& gameObjectName, const XMFLOAT4X4& world, const UINT& matIndex, const XMFLOAT4X4& texTransform)
{
	if (mInstanceCount == mInstanceDataCapacity) {
		// 应该进行扩容操作
		// 暂时不实现
		OutputMessageBox("Can not add new instance data!");
		return;
	}

	InstanceData instance;
	instance.World = world;
	XMMATRIX worldMatrix = XMLoadFloat4x4(&world);
	XMMATRIX inverseTransposeWorldMatrix = MathHelper::InverseTranspose(worldMatrix);
	XMStoreFloat4x4(&instance.InverseTransposeWorld, XMMatrixTranspose(inverseTransposeWorldMatrix));
	instance.MaterialIndex = matIndex;
	instance.TexTransform = texTransform;

	mInstances[gameObjectName] = instance;
	mIndices[gameObjectName] = mInstanceCount;
	mNumFramesDirties[gameObjectName] = gNumFrameResources;

	++mInstanceCount;
}

void Instance::InstanceDataChange(const std::string& gameObjectName)
{
	mNumFramesDirties[gameObjectName] = gNumFrameResources;
}

void Instance::UpdateInstanceData()
{
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];

	for (auto& p : mInstances) {
		if (mNumFramesDirties[p.first] > 0) {

			XMMATRIX world = XMLoadFloat4x4(&p.second.World);
			XMMATRIX inverseTransposeWorld = XMLoadFloat4x4(&p.second.InverseTransposeWorld);
			XMMATRIX texTransform = XMLoadFloat4x4(&p.second.TexTransform);

			InstanceData instanceData;
			XMStoreFloat4x4(&instanceData.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&instanceData.InverseTransposeWorld, XMMatrixTranspose(inverseTransposeWorld));
			XMStoreFloat4x4(&instanceData.TexTransform, XMMatrixTranspose(texTransform));
			instanceData.MaterialIndex = p.second.MaterialIndex;

			uploadBuffer->CopyData(mIndices[p.first], instanceData);

			--mNumFramesDirties[p.first];
		}
	}
}

void Instance::Draw(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &mMesh->VertexBufferView);
	cmdList->IASetIndexBuffer(&mMesh->IndexBufferView);
	cmdList->IASetPrimitiveTopology(mMesh->PrimitiveType);

	auto instanceBuffer = mFrameResources[gCurrFrameResourceIndex]->Resource();
	cmdList->SetGraphicsRootShaderResourceView(0, instanceBuffer->GetGPUVirtualAddress());

	cmdList->DrawIndexedInstanced(mMesh->IndexCount, mInstanceCount, 0, 0, 0);
}

