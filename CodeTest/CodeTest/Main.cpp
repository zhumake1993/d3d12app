#include <iostream>
#include <string>
#include <Windows.h>
#include "test.h"

using namespace std;

void kk() {
	gtest->tt();
}

int main()
{
	std::unique_ptr<test> gtest = std::make_unique<test>();
	gtest->tt();

	cout << "hellow" << endl;

	return 0;
}