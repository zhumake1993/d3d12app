#pragma once

#include "../Manager/GameObjectManager.h"

class WirefenceBox :
	public GameObject
{
public:
	WirefenceBox(std::shared_ptr<CommonResource> commonResource);
	~WirefenceBox();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};