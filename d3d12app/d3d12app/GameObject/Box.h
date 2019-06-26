#pragma once

#include "../Manager/GameObjectManager.h"

class Box :
	public GameObject
{
public:
	Box();
	~Box();

private:
	virtual void Update(const GameTimer& gt)override;

public:


private:

};