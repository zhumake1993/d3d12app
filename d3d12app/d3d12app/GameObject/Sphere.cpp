#include "Sphere.h"

Sphere::Sphere(std::shared_ptr<CommonResource> commonResource)
	:GameObject(commonResource)
{
	mMatName = "mirror";

	mMeshName = "sphere";

	mRenderLayer = (int)RenderLayer::Opaque;
}

Sphere::~Sphere()
{
}

void Sphere::Update(const GameTimer& gt)
{
}
