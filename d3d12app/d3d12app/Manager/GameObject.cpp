#include "GameObject.h"

GameObject::GameObject(std::shared_ptr<CommonResource> commonResource)
{
	mCommonResource = commonResource;
}

GameObject::~GameObject()
{
}

std::shared_ptr<MeshManager> GameObject::GetMeshManager()
{
	return std::static_pointer_cast<MeshManager>(mCommonResource->mMeshManager);
}

std::shared_ptr<InstanceManager> GameObject::GetInstanceManager()
{
	return std::static_pointer_cast<InstanceManager>(mCommonResource->mInstanceManager);
}

std::shared_ptr<MaterialManager> GameObject::GetMaterialManager()
{
	return std::static_pointer_cast<MaterialManager>(mCommonResource->mMaterialManager);
}

bool GameObject::GetKeyDown(int key)
{
	return std::static_pointer_cast<InputManager>(mCommonResource->mInputManager)->GetKeyDown(key);
}

bool GameObject::GetKeyPress(int key)
{
	return std::static_pointer_cast<InputManager>(mCommonResource->mInputManager)->GetKeyPress(key);
}

bool GameObject::GetKeyUp(int key)
{
	return std::static_pointer_cast<InputManager>(mCommonResource->mInputManager)->GetKeyUp(key);
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

