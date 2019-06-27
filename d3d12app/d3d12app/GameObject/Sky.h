#pragma once

#include "../Manager/GameObjectManager.h"

class Sky :
	public GameObject
{
public:
	Sky(std::shared_ptr<CommonResource> commonResource);
	~Sky();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};