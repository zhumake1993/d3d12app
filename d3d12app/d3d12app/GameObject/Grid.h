#pragma once

#include "../Manager/GameObjectManager.h"

class Grid :
	public GameObject
{
public:
	Grid(std::shared_ptr<CommonResource> commonResource);
	~Grid();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};