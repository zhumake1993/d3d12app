#include "Cylinder.h"

Cylinder::Cylinder(std::shared_ptr<CommonResource> commonResource)
	:GameObject(commonResource)
{
	mMatName = "bricks";
	XMStoreFloat4x4(&mTexTransform, XMMatrixScaling(1.5f, 2.0f, 1.0f));

	mMeshName = "cylinder";

	mRenderLayer = (int)RenderLayer::Opaque;
}

Cylinder::~Cylinder()
{
}

void Cylinder::Update(const GameTimer& gt)
{
}
