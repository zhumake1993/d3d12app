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

std::shared_ptr<Mesh> Instance::GetMesh()
{
	return mMesh;
}

void Instance::CalculateBoundingBox()
{
	auto vertices = (Vertex*)mMesh->VertexBufferCPU->GetBufferPointer();
	auto size = mMesh->VertexBufferByteSize / mMesh->VertexByteStride;
	BoundingBox::CreateFromPoints(mBounds, size, &vertices[0].Pos, sizeof(Vertex));
}

void Instance::AddInstanceData(const std::string& gameObjectName, const XMFLOAT4X4& world, const UINT& matIndex, const XMFLOAT4X4& texTransform)
{
	if (mInstanceCount == mInstanceDataCapacity) {
		// Ӧ�ý������ݲ���
		// ��ʱ��ʵ��
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

	++mInstanceCount;
}

void Instance::UpdateInstanceData(std::shared_ptr<Camera> camera)
{
	auto& uploadBuffer = mFrameResources[gCurrFrameResourceIndex];

	XMMATRIX view = camera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	mVisibleCount = 0;
	for (auto& p : mInstances) {
		XMMATRIX world = XMLoadFloat4x4(&p.second.World);
		XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);
		XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

		// ��ƽ��ͷ��������ռ�ת��������ľֲ�����ռ�
		BoundingFrustum localSpaceFrustum;
		camera->mCamFrustum.Transform(localSpaceFrustum, viewToLocal);

		// ƽ��ͷ�޳�
		if ((localSpaceFrustum.Contains(mBounds) != DirectX::DISJOINT) || (camera->mFrustumCullingEnabled == false)) {

			XMMATRIX world = XMLoadFloat4x4(&p.second.World);
			XMMATRIX inverseTransposeWorld = XMLoadFloat4x4(&p.second.InverseTransposeWorld);
			XMMATRIX texTransform = XMLoadFloat4x4(&p.second.TexTransform);

			InstanceData instanceData;

			XMStoreFloat4x4(&instanceData.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&instanceData.InverseTransposeWorld, XMMatrixTranspose(inverseTransposeWorld));
			XMStoreFloat4x4(&instanceData.TexTransform, XMMatrixTranspose(texTransform));
			instanceData.MaterialIndex = p.second.MaterialIndex;

			uploadBuffer->CopyData(mVisibleCount++, instanceData);
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

	cmdList->DrawIndexedInstanced(mMesh->IndexCount, mVisibleCount, 0, 0, 0);
}

