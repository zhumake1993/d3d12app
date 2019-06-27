#include "InputManager.h"

InputManager::InputManager()
{
	Clear();
}

InputManager::~InputManager()
{
}

void InputManager::Update(const GameTimer& gt)
{
	for (int i = 0; i < size; i++) {
		mKeyDown[i] = false;
		mKeyUp[i] = false;
	}
}

void InputManager::OnKeyDown(int key)
{
	if (mKeyPress[key] == false) {
		mKeyDown[key] = true;
		mKeyPress[key] = true;
	}
}

void InputManager::OnKeyUp(int key)
{
	mKeyUp[key] = true;
	mKeyPress[key] = false;
}

bool InputManager::GetKeyDown(int key)
{
	return mKeyDown[key];
}

bool InputManager::GetKeyPress(int key)
{
	return mKeyPress[key];
}

bool InputManager::GetKeyUp(int key)
{
	return mKeyUp[key];
}

void InputManager::Clear()
{
	for (int i = 0; i < size; i++) {
		mKeyDown[i] = false;
		mKeyPress[i] = false;
		mKeyUp[i] = false;
	}
}
