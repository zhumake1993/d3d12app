#pragma once

#include "d3dUtil.h"
#include "GeometryGenerator.h"

struct Mesh
{
	std::string Name;

	// 使用ID3DBlob，因为顶点/索引数据的格式是未定的
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;
	UINT IndexCount = 0;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	DirectX::BoundingBox Bounds;
};

class MeshManager
{
public:
	MeshManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	~MeshManager();

	void AddGeometry(std::string Name,  GeometryGenerator::MeshData mesh);
	void AddGeometry(std::string Name, std::vector<Vertex> &vertices, std::vector<std::uint16_t> &indices);
	void DeleteGeometry(std::string Name);

private:

public:
	std::unordered_map<std::string, std::unique_ptr<Mesh>> mGeometries;

private:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList* mCmdList;
};