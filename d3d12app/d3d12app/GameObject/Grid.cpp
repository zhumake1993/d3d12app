#include "Grid.h"

Grid::Grid(std::shared_ptr<CommonResource> commonResource)
	:GameObject(commonResource)
{
	mGameObjectName = "grid";

	mMatName = "tile";
	XMStoreFloat4x4(&mTexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));

	mMeshName = "grid";

	mRenderLayer = (int)RenderLayer::Opaque;
}

Grid::~Grid()
{
}

void Grid::Update(const GameTimer& gt)
{
}
