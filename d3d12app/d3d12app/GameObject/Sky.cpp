#include "Sky.h"

Sky::Sky(std::shared_ptr<CommonResource> commonResource)
	:GameObject(commonResource)
{
	mGameObjectName = "sky";

	mScale = XMFLOAT3(5000.0f, 5000.0f, 5000.0f);

	mMatName = "sky";

	mMeshName = "sphere";

	mRenderLayer = (int)RenderLayer::Sky;
}

Sky::~Sky()
{
}

void Sky::Update(const GameTimer& gt)
{
}
