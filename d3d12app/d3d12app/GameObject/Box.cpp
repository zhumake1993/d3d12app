#include "Box.h"

Box::Box()
	:GameObject()
{
	mGameObjectName = "box";

	mTranslation = XMFLOAT3(0.0f, 0.5f, 0.0f);
	mScale = XMFLOAT3(2.0f, 1.0f, 2.0f);

	mMatName = "bricks2";
	XMStoreFloat4x4(&mTexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));

	mMeshName = "box";

	mRenderLayer = (int)RenderLayer::Opaque;
}

Box::~Box()
{
}

void Box::Update(const GameTimer& gt)
{
	//mTranslation = XMFLOAT3(3.0f * cos(gt.TotalTime()), 2.0f, 3.0f * sin(gt.TotalTime()));
	//mTranslation = XMFLOAT3(3.0f * cos(gt.TotalTime()*0.01), 2.0f, 3.0f * sin(gt.TotalTime()*0.01));
	if (GetAsyncKeyState('Z') & 0x8000)
		mRotation.y += gt.DeltaTime() * 0.2f;
}
