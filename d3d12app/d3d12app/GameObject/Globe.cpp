#include "Globe.h"

Globe::Globe(std::shared_ptr<CommonResource> commonResource)
	:GameObject(commonResource)
{
	mGameObjectName = "globe";

	mTranslation = XMFLOAT3(0.0f, 2.0f, 0.0f);
	mScale = XMFLOAT3(2.0f, 2.0f, 2.0f);

	mMatName = "mirror";

	mMeshName = "sphere";

	mRenderLayer = (int)RenderLayer::OpaqueDynamicReflectors;
}

Globe::~Globe()
{
}

void Globe::Update(const GameTimer& gt)
{
}
