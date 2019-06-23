#include "GameObjectManager.h"

GameObjectManager::GameObjectManager()
{
}

GameObjectManager::~GameObjectManager()
{
}

void GameObjectManager::SetInstanceManager(std::shared_ptr<InstanceManager> instanceManager)
{
	mInstanceManager = instanceManager;
}

void GameObjectManager::AddGameObject(std::unique_ptr<GameObject> gameObject)
{
	if (mGameObjects.find(gameObject->mGameObjectName) != mGameObjects.end()) {
		OutputMessageBox("GameObject already exists!");
		return;
	}

	mInstanceManager->AddInstance(gameObject->mGameObjectName, gameObject->GetWorld(),
		gameObject->mMatName, gameObject->mTexTransform, gameObject->mMeshName, gameObject->mRenderLayer);

	mGameObjects[gameObject->mGameObjectName] = std::move(gameObject);
}

void GameObjectManager::Update(const GameTimer& gt)
{
	for (auto &p : mGameObjects) {
		p.second->Update(gt);

		if (p.second->mInstanceDataChanged) {
			p.second->mInstanceDataChanged = false;
			mInstanceManager->InstanceDataChange(p.second->mGameObjectName, p.second->mMeshName, p.second->mRenderLayer);
		}
	}
}
