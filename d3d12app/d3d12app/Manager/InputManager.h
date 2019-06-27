#pragma once

#include "../Common/d3dUtil.h"
#include "Manager.h"

const int size = 256;

class InputManager :
	public Manager
{
public:
	InputManager();
	~InputManager();

	void Update(const GameTimer& gt);

	void OnKeyDown(int key);
	void OnKeyUp(int key);

	bool GetKeyDown(int key);
	bool GetKeyPress(int key);
	bool GetKeyUp(int key);

private:
	void Clear();

public:
	//

private:
	bool mKeyDown[size];
	bool mKeyPress[size];
	bool mKeyUp[size];
};
