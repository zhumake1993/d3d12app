#pragma once

#include "../Manager/GameObjectManager.h"

class Sphere :
	public GameObject
{
public:
	Sphere(std::shared_ptr<CommonResource> commonResource);
	~Sphere();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};