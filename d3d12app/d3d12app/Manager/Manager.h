#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/Camera.h"

class Manager
{
public:
	//

private:
	//

public:
	//

private:
	//
};

struct CommonResource {
	std::shared_ptr<Manager> mTextureManager;
	std::shared_ptr<Manager> mMaterialManager;
	std::shared_ptr<Manager> mMeshManager;
	std::shared_ptr<Manager> mInstanceManager;
	std::shared_ptr<Manager> mGameObjectManager;
	std::shared_ptr<Manager> mInputManager;

	std::shared_ptr<Camera> mCamera;
};