#pragma once

#include "../Manager/GameObjectManager.h"

class Skull :
	public GameObject
{
public:
	Skull(std::shared_ptr<CommonResource> commonResource);
	~Skull();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};