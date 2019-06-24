#pragma once

#include "../Common/d3dUtil.h"
#include "InstanceManager.h"
#include "MaterialManager.h"
#include "GameObject.h"

using namespace DirectX;

class GameObjectManager
{
public:
	GameObjectManager();
	~GameObjectManager();

	void SetInstanceManager(std::shared_ptr<InstanceManager> instanceManager);
	void SetMaterialManager(std::shared_ptr<MaterialManager> materialManager);

	void AddGameObject(std::unique_ptr<GameObject> gameObject);

	void Update(const GameTimer& gt);

private:
	std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;

public:
	//

private:
	std::shared_ptr<InstanceManager> mInstanceManager;
	std::shared_ptr<MaterialManager> mMaterialManager;
};