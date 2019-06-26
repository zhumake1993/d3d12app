#include "Skull.h"

Skull::Skull()
	:GameObject()
{
	mScale = XMFLOAT3(0.2f, 0.2f, 0.2f);
}

Skull::~Skull()
{
}

void Skull::Update(const GameTimer& gt)
{
	//mTranslation = XMFLOAT3(3.0f * cos(gt.TotalTime()), 2.0f, 3.0f * sin(gt.TotalTime()));
	mTranslation = XMFLOAT3(3.0f * cos(gt.TotalTime()*0.01), 2.0f, 3.0f * sin(gt.TotalTime()*0.01));
	mRotation = XMFLOAT3(0.0f, gt.TotalTime(), 0.0f);
}
