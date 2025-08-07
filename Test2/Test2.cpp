// Test2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <future>


void ClearParallel(std::vector<std::vector<std::wstring>>& data) {
	size_t threadCount = std::thread::hardware_concurrency();
	std::vector<std::future<void>> futures(threadCount);

	size_t dataSize = data.size();
	size_t chunkSize = dataSize / threadCount;

	for (size_t threadID = 0; threadID < threadCount; threadID++) {
		futures[threadID] = std::async(std::launch::async, [&data, threadID, chunkSize, threadCount, dataSize]() {
			size_t start = threadID * chunkSize;
			size_t end = (threadID + 1 == threadCount) ? dataSize : start + chunkSize;
			for (size_t row = start; row < end; ++row) {
				data[row].clear();
				data[row].shrink_to_fit();
			}
			});
	}

	for (auto& future : futures) {
		future.wait();
	}

	data.clear();
	data.shrink_to_fit();
}

int main()
{
	int maxRow = 50000000;
	std::vector<std::vector<std::wstring>> vDataStr;

	//v1
	//add - 7624ms
	auto start = std::chrono::system_clock::now();
	vDataStr.resize(maxRow);
	for (int row = 0; row < maxRow; row++) {
		vDataStr[row].resize(1);
		vDataStr[row][0] = std::to_wstring(row);
	}
	auto end = std::chrono::system_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

	//clear - 5111ms
	start = std::chrono::system_clock::now();
	vDataStr.clear();
	end = std::chrono::system_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;



	//v2
	//add std::async - 2640ms
	start = std::chrono::system_clock::now();
	vDataStr.resize(maxRow);
	size_t dataSize = vDataStr.size();
	size_t threadCount = std::thread::hardware_concurrency();
	std::vector<std::future<void>> futures(threadCount);
	for (size_t threadID = 0; threadID < threadCount; threadID++) {
		futures[threadID] = std::async(std::launch::async, [&vDataStr, threadID, threadCount, dataSize]()
			{
				size_t part = dataSize / threadCount, dataStart = threadID * part, dataNextThread = (threadID + 1 == threadCount) ? dataSize : dataStart + part;
				for (size_t row = dataStart; row < dataNextThread; ++row) {
					vDataStr[row].resize(1);
					vDataStr[row][0] = std::to_wstring(row);
				}
			});
	}

	for (auto& future : futures) {
		future.wait();
	}
	end = std::chrono::system_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

	//clear std::async - 58335mss
	start = std::chrono::system_clock::now();
	//vDataStr.clear();
	ClearParallel(vDataStr);
	end = std::chrono::system_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

	return 0;
}


//
////компилятору подать флаг оптимизации
////-O3
//#include <iostream>
//#include <vector>
//#include <chrono>
//#include <string>
//#include <future>
//#include <iomanip>
//
//struct Timer
//{
//    std::string tail;
//    decltype(std::chrono::system_clock::now()) start{};
//
//    Timer(std::string_view tail) :tail{ tail }, start{ std::chrono::system_clock::now() } {}
//    ~Timer()
//    {
//        auto end = std::chrono::system_clock::now();
//        std::cout << std::setw(4);
//        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//        std::cout << "ms" << tail;
//    }
//    //rof5 - лень
//};
//
//int main()
//{
//    for (int step = 0; step < 10; ++step)
//    {
//        using VECTYPE = std::vector<std::vector<std::wstring>>;
//        int maxRow = 50000000;
//
//        //v1 Вариант-1
//        {
//            VECTYPE vDataStr; //таблица
//
//            //add - 7624ms
//            {
//                Timer t(" res -");
//                vDataStr.resize(maxRow);
//                for (int row = 0; row < maxRow; row++)
//                {
//                    vDataStr[row].resize(1);
//                    vDataStr[row][0] = std::to_wstring(row);
//                }
//            }
//
//            //clear - 5111ms
//            {
//                Timer t(" clr //");
//                vDataStr.clear();
//            }
//        }
//
//
//        //v2 Вариант-2
//        {
//            VECTYPE vDataStr; //таблица
//
//            //add std::async - 2640ms
//            {
//                Timer t(" ares -");
//                vDataStr.resize(maxRow);
//                size_t dataSize = vDataStr.size();
//                size_t threadCount = std::thread::hardware_concurrency();
//                std::vector<std::future<void>> futures(threadCount);
//                for (size_t threadID = 0; threadID < threadCount; threadID++)
//                {
//                    futures[threadID] = std::async(std::launch::async, [&vDataStr, threadID, threadCount, dataSize]()
//                        {
//                            size_t part = dataSize / threadCount, dataStart = threadID * part, dataNextThread = (threadID + 1 == threadCount) ? dataSize : dataStart + part;
//                            for (size_t row = dataStart; row < dataNextThread; ++row)
//                            {
//                                vDataStr[row].resize(1);
//                                vDataStr[row][0] = std::to_wstring(row);
//                            }
//                        });
//                }
//
//                for (auto& future : futures)
//                {
//                    future.wait();
//                }
//            }
//
//            //clear std::async - 58335ms !!!
//            {
//                Timer t(" aclr\n");
//                vDataStr.clear();
//            }
//        }
//    }
//}