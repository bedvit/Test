// Test.cpp: определяет точку входа для консольного приложения.
// 
//тестирование скорости контейнеров для WC_LISTVIEWW

//#include "stdafx.h"
#include <Windows.h>
#include <stdexcept>//исключения
//#include <functional>

#include <iostream>
#include <future>
#include <unordered_map>
#include <map>


int VectorOrMap()
{
	size_t threadCount = 100000000;
	HWND res = (HWND)0;
	HWND resIn = (HWND)1;

	//version 1
	res = (HWND)0;
	std::vector<HWND> testArr = { (HWND)0,(HWND)1,(HWND)2,(HWND)3,(HWND)4,(HWND)5,(HWND)6,(HWND)7,(HWND)8,(HWND)9};
	std::chrono::steady_clock::time_point timeStart = std::chrono::steady_clock::now();
	for (size_t threadID = 0; threadID < threadCount; threadID++) {
		for (size_t i = 0; i < testArr.size(); i++) {
			if (testArr[i] == resIn) {
				res = (HWND)i;
				break;
			}
		}

	}
	std::chrono::steady_clock::time_point timeStop = std::chrono::steady_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds> (timeStop - timeStart).count();
	std::cout << '\n';
	std::cout << res;
	std::cout << '\n';

	//version 2
	res = (HWND)0;
	timeStart = std::chrono::steady_clock::now();
	std::unordered_map < HWND, std::vector<std::vector<std::wstring>> > unorderedMap = { { (HWND)1, { {L"Test1"} } },{ (HWND)2, { {L"Test2"} } },{ (HWND)3, { {L"Test3"} } },{ (HWND)4, { {L"Test4"} } },{ (HWND)5, { {L"Test5"} } },{ (HWND)6, { {L"Test6"} } },{ (HWND)7, { {L"Test7"} } },{ (HWND)8, { {L"Test8"} } },{ (HWND)9, { {L"Test9"} } },{ (HWND)0, { {L"Test0"} } } };
	for (size_t threadID = 0; threadID < threadCount; threadID++) {
		auto searchUnorderedMap = unorderedMap.find(resIn);
		if (searchUnorderedMap != unorderedMap.end()) {
			res = searchUnorderedMap->first;
		}
	}
	timeStop = std::chrono::steady_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds> (timeStop - timeStart).count();
	std::cout << '\n';
	std::cout << res;
	std::cout << '\n';
	
	//version 3
	res = 0;
	timeStart = std::chrono::steady_clock::now();
	std::map < HWND, std::vector<std::vector<std::wstring>> > map = { { (HWND)1, { {L"Test1"} } },{ (HWND)2, { {L"Test2"} } },{ (HWND)3, { {L"Test3"} } },{ (HWND)4, { {L"Test4"} } },{ (HWND)5, { {L"Test5"} } },{ (HWND)6, { {L"Test6"} } },{ (HWND)7, { {L"Test7"} } },{ (HWND)8, { {L"Test8"} } },{ (HWND)9, { {L"Test9"} } },{ (HWND)0, { {L"Test0"} } } };
	for (size_t threadID = 0; threadID < threadCount; threadID++) {
		auto searchMap = map.find(resIn);
		if (searchMap != map.end()) {
			res = searchMap->first;
		}
	}
	timeStop = std::chrono::steady_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds> (timeStop - timeStart).count();
	std::cout << '\n';
	std::cout << res;
	std::cout << '\n';

	system("pause");
	return 0;
}

int main() {

	//тестирование скорости контейнеров для WC_LISTVIEWW
	VectorOrMap();

	return 0;
}










