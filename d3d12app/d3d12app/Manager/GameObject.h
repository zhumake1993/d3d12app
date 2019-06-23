#pragma once

#include "../Common/d3dUtil.h"

using namespace DirectX;

class GameObject
{
	friend class GameObjectManager;

public:
	GameObject();
	~GameObject();

private:
	virtual void Update(const GameTimer& gt);

	XMFLOAT4X4 GetWorld();
	void InstanceDataChange();

public:
	std::string mGameObjectName = "GameObject";

	XMFLOAT3 mTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	std::string mMatName;
	XMFLOAT4X4 mTexTransform = MathHelper::Identity4x4();

	std::string mMeshName;

	int mRenderLayer = -1;

private:
	bool mInstanceDataChanged = true;
};