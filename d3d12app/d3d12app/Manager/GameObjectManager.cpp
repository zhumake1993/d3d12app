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

void GameObjectManager::SetMaterialManager(std::shared_ptr<MaterialManager> materialManager)
{
	mMaterialManager = materialManager;
}

void GameObjectManager::AddGameObject(std::unique_ptr<GameObject> gameObject)
{
	if (mGameObjects.find(gameObject->mGameObjectName) != mGameObjects.end()) {
		OutputMessageBox("GameObject already exists!");
		return;
	}

	mInstanceManager->AddInstance(gameObject->mGameObjectName, gameObject->GetWorld(),
		gameObject->mMatName, gameObject->mTexTransform, gameObject->mMeshName, gameObject->mRenderLayer);

	gameObject->mInstanceManager = mInstanceManager;
	gameObject->mMaterialManager = mMaterialManager;

	mGameObjects[gameObject->mGameObjectName] = std::move(gameObject);
}

void GameObjectManager::Update(const GameTimer& gt)
{
	for (auto &p : mGameObjects) {
		p.second->Update(gt);
	}
}
