#pragma once

#include "../Manager/GameObjectManager.h"

class Cylinder :
	public GameObject
{
public:
	Cylinder(std::shared_ptr<CommonResource> commonResource);
	~Cylinder();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};