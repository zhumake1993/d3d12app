#include "GameObjectManager.h"

GameObjectManager::GameObjectManager(std::shared_ptr<CommonResource> commonResource)
{
	mCommonResource = commonResource;
}

GameObjectManager::~GameObjectManager()
{
}

void GameObjectManager::AddGameObject(std::unique_ptr<GameObject> gameObject)
{
	if (mGameObjects.find(gameObject->mGameObjectName) != mGameObjects.end()) {
		OutputMessageBox("GameObject already exists!");
		return;
	}

	GetInstanceManager()->AddInstance(gameObject->mGameObjectName, gameObject->GetWorld(),
		gameObject->mMatName, gameObject->mTexTransform, gameObject->mMeshName, gameObject->mRenderLayer);

	mGameObjects[gameObject->mGameObjectName] = std::move(gameObject);
}

void GameObjectManager::Update(const GameTimer& gt)
{
	for (auto &p : mGameObjects) {
		p.second->Update(gt);

		GetInstanceManager()->UpdateInstance(p.second->mGameObjectName, p.second->GetWorld(),
			p.second->mMatName, p.second->mTexTransform, p.second->mMeshName, p.second->mRenderLayer);
	}
}

std::shared_ptr<InstanceManager> GameObjectManager::GetInstanceManager()
{
	return std::static_pointer_cast<InstanceManager>(mCommonResource->mInstanceManager);
}