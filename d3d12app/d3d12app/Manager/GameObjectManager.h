#pragma once

#include "GameObject.h"
#include "Manager.h"

using namespace DirectX;

class GameObjectManager :
	public Manager
{
public:
	GameObjectManager(std::shared_ptr<CommonResource> commonResource);
	~GameObjectManager();

	void AddGameObject(std::unique_ptr<GameObject> gameObject);

	void Update(const GameTimer& gt);

private:
	std::shared_ptr<InstanceManager> GetInstanceManager();

public:
	//

private:
	std::unordered_map<std::string, std::unique_ptr<GameObject>> mGameObjects;
	std::shared_ptr<CommonResource> mCommonResource;
};