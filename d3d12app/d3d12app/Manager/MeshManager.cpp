#include "MeshManager.h"

MeshManager::MeshManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	mDevice = device;
	mCmdList = cmdList;
}

MeshManager::~MeshManager()
{
	for (const auto& p : mGeometries) {
		p.second->VertexBufferUploader = nullptr;
		p.second->IndexBufferUploader = nullptr;
	}
}

void MeshManager::AddGeometry(std::string Name, GeometryGenerator::MeshData mesh)
{
	// 注意：Vertex与GeometryGenerator::Vertex是不一样的
	// 必须将mesh中的顶点向量和索引向量重新复制一份
	std::vector<Vertex> vertices(mesh.Vertices.size());
	for (size_t i = 0; i < mesh.Vertices.size(); ++i) {
		vertices[i].Pos = mesh.Vertices[i].Position;
		vertices[i].Normal = mesh.Vertices[i].Normal;
		vertices[i].TexC = mesh.Vertices[i].TexC;
		vertices[i].TangentU = mesh.Vertices[i].TangentU;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(mesh.GetIndices16()), std::end(mesh.GetIndices16()));

	AddGeometry(Name, vertices, indices);
}

void MeshManager::AddGeometry(std::string Name, std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
	if (mGeometries.find(Name) != mGeometries.end()) {
		OutputMessageBox("Mesh already exists!");
		return;
	}

	auto meshGeo = std::make_shared<Mesh>();
	meshGeo->Name = Name;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshGeo->VertexBufferCPU));
	CopyMemory(meshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshGeo->IndexBufferCPU));
	CopyMemory(meshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice,
		mCmdList, vertices.data(), vbByteSize, meshGeo->VertexBufferUploader);

	meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice,
		mCmdList, indices.data(), ibByteSize, meshGeo->IndexBufferUploader);

	meshGeo->VertexByteStride = sizeof(Vertex);
	meshGeo->VertexBufferByteSize = vbByteSize;
	meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	meshGeo->IndexBufferByteSize = ibByteSize;
	meshGeo->IndexCount = (UINT)indices.size();

	meshGeo->VertexBufferView.BufferLocation = meshGeo->VertexBufferGPU->GetGPUVirtualAddress();
	meshGeo->VertexBufferView.StrideInBytes = meshGeo->VertexByteStride;
	meshGeo->VertexBufferView.SizeInBytes = meshGeo->VertexBufferByteSize;

	meshGeo->IndexBufferView.BufferLocation = meshGeo->IndexBufferGPU->GetGPUVirtualAddress();
	meshGeo->IndexBufferView.Format = meshGeo->IndexFormat;
	meshGeo->IndexBufferView.SizeInBytes = meshGeo->IndexBufferByteSize;

	mGeometries[Name] = meshGeo;
}

void MeshManager::DeleteGeometry(std::string Name)
{
}
