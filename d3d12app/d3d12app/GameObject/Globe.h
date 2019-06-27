#pragma once

#include "../Manager/GameObjectManager.h"

class Globe :
	public GameObject
{
public:
	Globe(std::shared_ptr<CommonResource> commonResource);
	~Globe();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};