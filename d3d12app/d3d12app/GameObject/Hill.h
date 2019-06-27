#pragma once

#include "../Manager/GameObjectManager.h"

class Hill :
	public GameObject
{
public:
	Hill(std::shared_ptr<CommonResource> commonResource);
	~Hill();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};