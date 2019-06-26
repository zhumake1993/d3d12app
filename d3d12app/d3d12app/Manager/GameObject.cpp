#include "GameObject.h"

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

void GameObject::Update(const GameTimer& gt)
{
}

XMFLOAT4X4 GameObject::GetWorld()
{
	XMMATRIX T = XMMatrixTranslation(mTranslation.x, mTranslation.y, mTranslation.z);
	XMMATRIX R = XMMatrixRotationX(mRotation.x) * XMMatrixRotationY(mRotation.y) * XMMatrixRotationZ(mRotation.z);
	XMMATRIX S = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
	XMMATRIX W = S * R * T; // ×¢ÒâË³Ðò
	XMFLOAT4X4 mWorld;
	XMStoreFloat4x4(&mWorld, W);
	return mWorld;
}

