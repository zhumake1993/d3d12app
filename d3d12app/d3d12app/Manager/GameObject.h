#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"
#include "InstanceManager.h"
#include "MaterialManager.h"

using namespace DirectX;

class GameObject
{
	friend class GameObjectManager;

public:
	GameObject();
	virtual ~GameObject();

private:
	virtual void Update(const GameTimer& gt);

	XMFLOAT4X4 GetWorld();

public:
	std::string mGameObjectName = "GameObject";

	XMFLOAT3 mTranslation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	std::string mMatName;
	XMFLOAT4X4 mTexTransform = MathHelper::Identity4x4();

	std::string mMeshName;

	int mRenderLayer = -1;

protected:
	std::shared_ptr<InstanceManager> mInstanceManager;
	std::shared_ptr<MaterialManager> mMaterialManager;
};