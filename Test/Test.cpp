// Test.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <fstream>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <Windows.h>
#include <vector>
#include <exception>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <ppl.h>



size_t GetRowsCountCSVansi(PCTSTR file, bool fileFlagNoBuffering)
{
	// создаем события с автоматическим сбросом
	HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEndRead == NULL) { return GetLastError(); }

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;
	
	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	HANDLE hFile = CreateFile(
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		return -1;
	}
	//FlushFileBuffers(hFile); //Сбрасывает буферы указанного файла и вызывает запись всех буферизованных данных в файл.

	DWORD  dwBytesReadWork = 0;
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты

	char* notAlignBuf = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	char* find;// указатель для поиска
	size_t strCount = 1; //счетчик строк
	bool errHandleEOF = false;

	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
				// асинхронный ввод-вывод все еще происходит 
				// сделаем кое-что пока он идет 
				break;
			case ERROR_HANDLE_EOF:
				// мы достигли конца файла 
				// закроем файл после обработки данных в рабочем буфере
				errHandleEOF = true;
				break;
			default:
				// закрываем дескрипторы
				goto return1;
			}
		}

		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
		find = bufWork; //буфер
		find = strchr(find, '\n');
		while (find != NULL)
		{
			find = find + 1;
			strCount++;
			find = strchr(find, '\n');
		}
		if (errHandleEOF)
		{
			// мы достигли конца файла в ходе асинхронной операции	
			// закрываем дескрипторы
			goto return0;
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		// ждем, пока завершится асинхронная операция чтения// проверим результат работы асинхронного чтения 
		WaitForSingleObject(hEndRead, INFINITE);// INFINITE);

		// если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
		{
			// решаем что делать с кодом ошибки
			switch (dwError = GetLastError())
			{
			case ERROR_HANDLE_EOF: 
			{
				// мы достигли конца файла в ходе асинхронной операции	
				// закрываем дескрипторы
				goto return0;
			}
			// решаем что делать с другими случаями ошибок
			default:
				goto return1;
			}// конец процедуры switch (dwError = GetLastError())
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}
return0:
	// закрываем дескрипторы
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return strCount;
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return -1;
}

int GetRowCSVansi(PCTSTR file, int strNum, bool fileFlagNoBuffering)
{
		// создаем события с автоматическим сбросом
	HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEndRead == NULL) { return GetLastError(); }

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
	// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	HANDLE hFile = CreateFile(
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		return -1;
	}

	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты

	char* notAlignBuf = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	char* find;// указатель для поиска
	char* findNext;// указатель для поиска следующий
	int strCount = 1; //счетчик строк
	std::string strOut;
	DWORD  dwBytesReadWork;
	bool errHandleEOF = false;

	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		find= buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: {errHandleEOF = true; break; } // мы достигли конца файла читалкой ReadFile
			default: {goto return1; }// другие ошибки
			}
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		if (ui.QuadPart > 0)//если рабочий буфер заполнен
		{
			bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
			find = bufWork; //новый буфер
			do //считаем строки в буфере
			{
				if (strCount == strNum)
				{
					findNext = strchr(find, '\n');
					if (findNext == NULL)
					{
						strOut = strOut + std::string(find);
					}
					else
					{
						findNext++;
						strOut = strOut + std::string(find, size_t(findNext - find));
						goto return0;
					}
				}
				find = strchr(find, '\n');
				if (find != NULL)
				{
					find++;
					strCount++;
				}
			} while (find != NULL);

		}
		if (errHandleEOF){ goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl,&dwBytesRead, FALSE))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
				case ERROR_HANDLE_EOF: {goto return0; }// мы достигли конца файла в ходе асинхронной операции
				default: {goto return1; }// другие ошибки
			}// конец процедуры switch (dwError = GetLastError())
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}

return0:
	// закрываем дескрипторы
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return 0;
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return -1;
}

//int FindRowsInCSVansi0(PCTSTR file, const char* findStr, bool multiLine, bool fileFlagNoBuffering)
//{
//
//// создаем события с автоматическим сбросом
//HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
//if (hEndRead == NULL) { return GetLastError(); }
//
//_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
//ui.QuadPart = 0;
//
//OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
//ovl.Offset = 0;         // младшая часть смещения равна 0
//ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
//ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения
//
//// открываем файл для чтения
//HANDLE hFile = CreateFile(	// дескриптор файла
//	file,   // имя файла
//	GENERIC_READ,          // чтение из файла
//	FILE_SHARE_READ,       // совместный доступ к файлу
//	NULL,                  // защиты нет
//	OPEN_EXISTING,         // открываем существующий файл
//	FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
//	NULL                   // шаблона нет
//);
//// проверяем на успешное открытие
//if (hFile == INVALID_HANDLE_VALUE)
//{
//	CloseHandle(hEndRead);
//	return -1;
//}
//
//const DWORD  nNumberOfBytesToRead = 16777216;//67108864;//33554432; //16777216;//8388608;//читаем в буфер байты
//char* notAlignBuf = new char[nNumberOfBytesToRead + 4096]; //буфер
//char* buf = notAlignBuf; //буфер
//if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 
//
//char* notAlignBufWork = new char[nNumberOfBytesToRead + 4096 + 1]; //буфер Рабочий
//char* bufWork = notAlignBufWork; //буфер
//if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  
//
//bufWork[0] = '\0';//добавим нуль-терминатор
//bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
//char* find;// указатель для поиска
//size_t strCount = 1; //счетчик строк
//size_t findStrLen = strlen(findStr); //счетчик строк
//std::string strOut; //итоговая строка
//DWORD dwBytesReadWork;
//DWORD findStatus = 0; //статус поиска
//ULONGLONG ignoreOffset = 0; //для расчета холостых циклов
//bool errHandleEOF = false; //метка конца файла
//
//// читаем данные из файла
//for (;;)
//{
//	DWORD  dwBytesRead;
//	DWORD  dwError;
//	find = buf; //буфер
//	// читаем одну запись
//	if (!ReadFile(
//		hFile,           // дескриптор файла
//		buf,             // адрес буфера, куда читаем данные
//		nNumberOfBytesToRead,// количество читаемых байтов
//		&dwBytesRead,    // количество прочитанных байтов
//		&ovl             // чтение асинхронное
//	))
//	{
//		switch (dwError = GetLastError())// решаем что делать с кодом ошибки
//		{
//			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
//		case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
//		case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
//		default: {goto return1; }// другие ошибки
//		}
//	}
//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//	if (ui.QuadPart != ignoreOffset)//игнорируем первую итерацию и этерации прошедшие следующим буфером, когда нужно было вернутся в предыдущий буфер
//	{
//		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
//		find = bufWork; //новый буфер
//	go_1:
//		char* strStart = bufWork;
//		char* strEnd = NULL;
//
//		if (findStatus == 0)//goFind
//		{
//			find = strstr(find, findStr);
//			if (find != NULL) //если нужная подстрока найдена
//			{
//
//				for (strStart = find; strStart >= bufWork; strStart--)
//				{
//					if (*strStart == '\n') { break; } //если нашли начало строки
//				}
//				strStart++;
//				if (ui.QuadPart == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
//				{
//					strEnd = strchr(find, '\n');
//					if (strEnd != NULL) //если нашли конец строки
//					{
//						strOut = strOut + std::string(strStart, strEnd - strStart + 1);
//						if (!multiLine)
//						{
//							goto return0;
//						}
//						else//поиск в следующей строке начинаем с конца предыдущей
//						{
//							find = strEnd++;
//							findStatus = 0;
//							goto go_1;
//						}
//					}
//					else//если конец строки в следующем буфере или конец файла
//					{
//						strOut = strOut + std::string(strStart);
//						findStatus = 1;
//					}
//				}
//				else //если начало строки не найдено и это не первый буфер смотрим предыдущий буфер
//				{
//					findStatus = 2;
//					ui.QuadPart -= (nNumberOfBytesToRead) * 3;
//					ignoreOffset = ui.QuadPart + nNumberOfBytesToRead;
//				}
//			} //если нужная подстрока найдена
//		}
//		else if (findStatus == 1)//goBuf
//		{
//			strEnd = strchr(find, '\n');
//			if (strEnd != NULL) //если нашли конец строки
//			{
//				strOut = strOut + std::string(find, strEnd - find+1);
//				findStatus = 0;
//				if (!multiLine)
//				{
//					goto return0;
//				}
//				else//поиск в следующей строке начинаем с конца предыдущей
//				{
//					find = strEnd++;
//					findStatus = 0;
//					goto go_1;
//				}
//			}
//			else//если конец строки в следующем буфере или конец файла
//			{
//				strOut = strOut + std::string(find);
//			}
//		}
//		else if (findStatus == 2)//goBackBuf
//		{
//
//			for (strStart = bufWork + dwBytesReadWork; strStart >= bufWork; strStart--)
//			{
//				if (*strStart == '\n') { break; }//если нашли начало строки
//			}
//			strStart++;
//			if (ui.QuadPart == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
//			{
//				find = strStart++;
//				findStatus = 1;
//				goto go_1;
//			}
//			else
//			{
//				ui.QuadPart -= nNumberOfBytesToRead * 3;//если начало строки не найдлено смотрим предыдущий буфер
//				ignoreOffset = ui.QuadPart + nNumberOfBytesToRead;
//			}
//		}
//	}
//	if (errHandleEOF) { if (findStatus != 2) goto return0; }
//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//
//	// ждем, пока завершится асинхронная операция чтения
//	WaitForSingleObject(hEndRead, INFINITE);
//
//	// проверим результат работы асинхронного чтения // если возникла проблема ... 
//	if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
//	{
//		switch (dwError = GetLastError())// решаем что делать с кодом ошибки
//		{
//		case ERROR_HANDLE_EOF: 
//		{
//			if (findStatus != 2) goto return0; break;//если конец файла. но мы должны вернутся найти начало строки.
//		}// мы достигли конца файла в ходе асинхронной операции
//		default: {goto return1; }// другие ошибки
//		}// конец процедуры switch (dwError = GetLastError())
//	}
//
//	//меняем буфер
//	char* bufTmp = bufWork;
//	bufWork = buf;
//	buf = bufTmp;
//
//	// увеличиваем смещение в файле
//	dwBytesReadWork = dwBytesRead;//кол-во считанных байт
//	ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
//	ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
//	ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
//}
//
//return0:
//// закрываем дескрипторы, освобождаем память
//std::cout << "String Find\n" << strOut << std::endl;
//CloseHandle(hFile);
//CloseHandle(hEndRead);
//delete[] notAlignBuf;
//delete[] notAlignBufWork;
//return 0;
//return1:
//CloseHandle(hFile);
//CloseHandle(hEndRead);
//delete[] notAlignBuf;
//delete[] notAlignBufWork;
//return -1;
//}

int createfile() //создание файла для теста
{
	const int N = 4000000; // количество строк в файле
	std::ofstream fout("C:\\CSV_1_GB.csv");
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	if (!fout){	return 1; }

	for (int i = 0; i < N; ++i)
	{
		fout << i + 1;
		fout << str << std::endl;
	}
	fout.close();
	return 0;
}

#pragma warning(disable : 4146)
std::string XLAT(std::string s)
{
	clock_t t1;
	clock_t t2;

	size_t MAXSTR = 1024;
	const char* const NAMEFILE = "C:\\CSV_1_GB.csv";
	t1 = clock();
	std::ifstream is(NAMEFILE, std::ios::in | std::ios::binary);

	size_t size = 100000;
	std::string str; str.assign(size, ' ');

	//std::string s(findStr);
	std::streamoff offset = -s.size();

	std::string sOut;

	while (is)
	{
		is.read((char*)str.data(), size);
		is.seekg(offset, std::ios::cur);

		size_t found = str.find(s);
		if (found != std::string::npos)
		{
			//std::cout << "Find OK: ";

			if (size - found < MAXSTR) // +
			{
				is.seekg(MAXSTR - size, std::ios::cur);
				is.read((char*)str.data(), size);
				found -= MAXSTR;
			}
			else if (size - found > size - MAXSTR) // -
			{
				is.seekg(-MAXSTR - size, std::ios::cur);
				is.read((char*)str.data(), size);
				found += MAXSTR;
			}

			size_t s1 = str.rfind('\n', found);
			size_t s2 = str.find('\n', found);
			//std::cout << str.substr(s1, s2 - s1) << std::endl;
			sOut = sOut + str.substr(s1+1, s2 - s1);
			return sOut;
		};
	}
	is.close();
	//std::cout << sOut << std::endl;
	//printf("   read + Find + seekg     : Time = %f(школа)\n\n",
	//	(clock() - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	return sOut;
}

std::string FindRowsInCSVansi(PCTSTR file, char* findStr, bool multiLine)
{
	const DWORD  nNumberOfBytesToRead = 16777216;//67108864;//33554432; //16777216;//8388608;//читаем в буфер байты
	size_t findStrLen = strlen(findStr);
	if (findStrLen >= nNumberOfBytesToRead) { return ""; };

	// создаем события с автоматическим сбросом
	HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
	//if (hEndRead == NULL) { return GetLastError(); }
	if (hEndRead == NULL) {	return ""; 	}

	// открываем файл для чтения
	HANDLE hFile = CreateFile(	// дескриптор файла
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS,//(fileFlagNoBuffering != 0 ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		return "";
	}

	char* notAlignBuf = new char[nNumberOfBytesToRead *2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  
	
	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	DWORD dwBytesReadWork = 0;
	DWORD findStatus = 0; //статус поиска
	bool errHandleEOF = false; //метка конца файла
	char* find;// указатель для поиска
	char* strStart;
	char* strEnd;
	char* bufWorkNew = bufWork;//буфер с учетом полной строки
	size_t strStartLen = 0;
	size_t strCount = 1; //счетчик строк
	std::string strOut; //итоговая строка

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// читаем данные из файла

	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
			default: {goto return1; }// другие ошибки
			}
		}

		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
	goNextFind: //если ищем вторую и последующие строки в этом же буфере
		if (findStatus == 0)//goFind
		{
			find = strstr(bufWorkNew, findStr);
			if (find != NULL) //если нужная подстрока найдена
			{
				for (strStart = find; strStart >= bufWorkNew; strStart--)
				{
					if (*strStart == '\n') { break; } //если нашли начало строки
				}
				strStart++; //не учитываем '\n'

				strEnd = strchr(find, '\n'); //ищем конец строки
				if (strEnd != NULL) //если нашли конец строки
				{
					strOut = strOut + std::string(strStart, strEnd - strStart + 1);
					if (!multiLine)
					{
						goto return0;
					}
					else//поиск в следующей строке начинаем с конца предыдущей
					{
						bufWorkNew = strEnd++;
						findStatus = 0;
						goto goNextFind;
					}
				}
				else//если конец строки в следующем буфере или конец файла
				{
					strOut = strOut + std::string(strStart);
					findStatus = 1;
				}
			} //если нужная подстрока найдена
		}
		else if (findStatus == 1)//goBuf
		{
			strEnd = strchr(bufWork, '\n');
			if (strEnd != NULL) //если нашли конец строки
			{
				strOut = strOut + std::string(bufWork, strEnd - bufWork + 1);
				findStatus = 0;
				if (!multiLine)
				{
					goto return0;
				}
				else//поиск в следующей строке начинаем с конца предыдущей
				{
					bufWorkNew = strEnd++;
					findStatus = 0;
					goto goNextFind;
				}
			}
			else//если конец строки в следующем буфере или конец файла
			{
				strOut = strOut + std::string(bufWork);
			}
		}

		//блок дозагрузки рабочего буфера межбуферной строкой
		char* bufWorkEnd = bufWork + dwBytesReadWork; //конец буфера
		for (strStart = bufWorkEnd; strStart >= bufWork; strStart--) { if (*strStart == '\n') { break; } } //если нашли начало строки
		if (strStart < bufWork && ui.QuadPart>=nNumberOfBytesToRead) { goto return1; }; //если строка больше буфера (не найден разделитель), кроме первого буфера
		strStartLen = (bufWorkEnd - ++strStart); //размер буфера, который добавляем к рабочему
		//пишем начало строки в новый буфер
		bufWorkNew = buf - strStartLen;//bufWorkNew = bufWork - strStartLen;
		memcpy(bufWorkNew, strStart, strStartLen);
		//

		if (errHandleEOF) { goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_HANDLE_EOF:{ goto return0; break; }// мы достигли конца файла в ходе асинхронной операции
			default: {goto return1; }// другие ошибки
			}
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;
		//

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}

return0:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return strOut;
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return "";
}


bool CompareCharPtrEqual(char* lhs, char* rhs) {
	if (lhs == NULL && rhs == NULL) return true;
	if (lhs == NULL || rhs == NULL) return false;
	return strcmp(lhs, rhs) == 0;
}
bool CompareCharPtrAscending(char* lhs, char* rhs) {
	if (lhs == NULL) return false;
	if (rhs == NULL) return true;
	return strcmp(lhs, rhs) < 0;
}
bool CompareCharPtrDescending(char* lhs, char* rhs) {
	if (lhs == NULL) return false;
	if (rhs == NULL) return true;
	return strcmp(lhs, rhs) > 0;
}
int SortDeleteDuplicateRowsCSVansi(LPCWSTR FileIn, LPCWSTR FileOut, int HeaderRowsCount, int OnlySort, int fileFlagNoBuffering)
{
	clock_t t1 = clock();
	int RowsCountOut = -1;
	// создаем события с автоматическим сбросом
	HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
	if (hEndRead == NULL){	return RowsCountOut;	}

	// открываем файл для чтения
	HANDLE hFile = CreateFile(	// дескриптор файла
		FileIn,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering != 0 ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		return RowsCountOut;
	}

	DWORD size = GetFileSize(hFile, NULL);
	const DWORD  nNumberOfBytesToRead = 16777216;//33554432; //16777216;//8388608;//читаем в буфер байты
	char* notAlignBuf = new char[size + 4096 + 1+ nNumberOfBytesToRead]; //4096 - выравнивание, 1 - '\0', nNumberOfBytesToRead - для последнего буфера (остаток может быть меньше, но буфер мы должны выдилить полный)
	char* buf = notAlignBuf; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	DWORD dwBytesReadWork = 0;
	DWORD  dwError;
	bool errHandleEOF = false; //метка конца файла
	char* find;// указатель для поиска
	char* nextBuf = buf;
	std::vector<char*> charPtrArr;
	charPtrArr.push_back(buf);

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// читаем данные из файла

	for (;;)
	{
		DWORD  dwBytesRead;
		//find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			nextBuf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
				//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
			default: {
				goto return1; 
			}// другие ошибки
			}
		}
		//if (dwBytesReadWork > 0) {
		//	find = nextBuf - dwBytesReadWork;
		//	nextBuf[dwBytesReadWork] = '\0';
		//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		//	find = strchr(find, '\n');
		//	while (find != NULL)
		//	{
		//		find[0] = '\0';
		//		//find = find + 1;
		//		charPtrArr.push_back(++find);
		//		find = strchr(find, '\n');
		//	}
		//}

		if (errHandleEOF) { goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
			default: {
				goto return1; 
			}// другие ошибки
			}// конец процедуры switch (dwError = GetLastError())
		}

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		nextBuf += dwBytesRead; //добавляем смещение к указателю на буфер
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	return RowsCountOut;

return0:
	clock_t t2 = clock();
	printf("Load: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	nextBuf[0] = '\0'; //конец буфера
	find = buf;
	find = strchr(find, '\n');
	while (find != NULL)
	{
		find[0] = '\0';
		charPtrArr.push_back(++find);
		find = strchr(find, '\n');
	}
	//char* tmp = charPtrArr[charPtrArr.size() - 2];
	if (strlen(charPtrArr[charPtrArr.size() - 1]) == 0) {	charPtrArr.pop_back(); 	}
	clock_t t22 = clock();
	printf("Load array: Time - %f\n", (t22 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	concurrency::parallel_buffered_sort(charPtrArr.begin()+ HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscending);
	clock_t t3 = clock();
	printf("Sort: Time - %f\n", (t3 - t22 + .0) / CLOCKS_PER_SEC); // время отработки

	clock_t t33 = clock();
	if (OnlySort == 0) 
	{
		std::vector<char*>::iterator it;
		it = std::unique(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrEqual);
		charPtrArr.resize(std::distance(charPtrArr.begin(), it));
	t33 = clock();
	printf("dell: Time - %f\n", (t33 - t3 + .0) / CLOCKS_PER_SEC); // время отработки
	}
	RowsCountOut = charPtrArr.size();

	CloseHandle(hFile);
	   

	//вар1
	std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
	if (!file2.is_open()) {	
		CloseHandle(hEndRead);
		delete[] notAlignBuf;
		return RowsCountOut;
	}
	std::ostream_iterator<char*> output_iterator(file2, "\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(charPtrArr.begin(), charPtrArr.end(), output_iterator);
	file2.close();



	////вар2
	//// открываем файл для чтения
	//hFile = CreateFile(	// дескриптор файла
	//	FileOut,   // имя файла
	//	GENERIC_WRITE,          // запись в файл
	//	FILE_SHARE_READ,       // совместный доступ к файлу
	//	NULL,                  // защиты нет
	//	CREATE_ALWAYS,         // перезаписывает или создает новый
	//	FILE_FLAG_RANDOM_ACCESS,//FILE_FLAG_SEQUENTIAL_SCAN,//FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering != 0 ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
	//	NULL                   // шаблона нет
	//);
	//// проверяем на успешное открытие
	//if (hFile == INVALID_HANDLE_VALUE)
	//{
	//	CloseHandle(hEndRead);
	//	delete[] notAlignBuf;
	//	return RowsCountOut;
	//}

	//DWORD dwBytesWritten;
	//DWORD nNumberOfBytesToWrite;
	//DWORD nNumberOfBytesToWriteOut=0;
	//std::string Out("");
	//Out.reserve(1127000000);

	//for (int i = 0; i < charPtrArr.size(); i++)
	//{

	//	nNumberOfBytesToWrite = strlen(charPtrArr[i]) + 1;
	//	charPtrArr[i][nNumberOfBytesToWrite - 1] = '\n';
	//	nNumberOfBytesToWriteOut += nNumberOfBytesToWrite;
	//	Out.append(charPtrArr[i], nNumberOfBytesToWrite);

	//	//if (!WriteFile(
	//	//	hFile,                    // дескриптор файла
	//	//	charPtrArr[i],                // буфер данных
	//	//	nNumberOfBytesToWrite,     // число байтов для записи
	//	//	&dwBytesWritten,  // число записанных байтов
	//	//	NULL          // асинхронный буфер
	//	//)
	//	//	) {
	//	//	switch (dwError = GetLastError())// решаем что делать с кодом ошибки
	//	//	{
	//	//		//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
	//	//	//case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
	//	//	case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
	//	//	default: {
	//	//		goto return1;
	//	//	}// другие ошибки
	//	//	}

	//	//}
	//}

	//if (!WriteFile(
	//	hFile,                    // дескриптор файла
	//	Out.c_str(),                // буфер данных
	//	nNumberOfBytesToWriteOut,     // число байтов для записи
	//	&dwBytesWritten,  // число записанных байтов
	//	NULL          // асинхронный буфер
	//)
	//	) {
	//	switch (dwError = GetLastError())// решаем что делать с кодом ошибки
	//	{
	//		//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
	//	//case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
	//	case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
	//	default: {
	//		goto return1;
	//	}// другие ошибки
	//	}

	//}
	//CloseHandle(hFile);
	////


	clock_t t4 = clock();
	printf("save: Time - %f\n", (t4 - t3 + .0) / CLOCKS_PER_SEC); // время отработки
	//
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	return RowsCountOut;

}


int main0()
{
	DWORD t1 = GetTickCount();

	HANDLE hFile = CreateFile(TEXT("C:\\CSV_1_GB.csv"),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Error: CreateFile = INVALID_HANDLE_VALUE\n");
		return -1;
	}

	//LPVOID buf = 0;
	//DWORD size = 0;
	DWORD dw;

	DWORD size = GetFileSize(hFile, NULL);

	//buf = HeapAlloc(GetProcessHeap(), 0, size);
	char *buf = new char[size];

	if (!buf)
	{
		printf("Error: HeapAlloc = %d\n", GetLastError());
		return -1;
	}

	if (!ReadFile(hFile, buf, size, &dw, NULL))
	{
		printf("Error: ReadFile = %d\n", GetLastError());
		return -1;
	}

	DWORD t2 = GetTickCount();

	//printf("vector size = %d, loading time = %d ms.\n", size, t2 - t1);

	//(GetProcessHeap(), 0, buf);
	delete[] buf;
	CloseHandle(hFile);
	//system("pause");
	return 0;
}

int main()
{
	//0.53
	//_ULARGE_INTEGER 0.53
	//memcpy 0.76 (+0.23)
	//nNumberOfBytesToRead*2 0.76
	//strStart 0.77
	//strrchr 1.08
	//strStartLen+memcpy 0.78
	//find = strstr(bufWorkNew, findStr) 1,30
	//const char* findCh = "917939";//"4000000";// "917939";
	//2.69 - FILE_FLAG_NO_BUFFERING
	//3.33
	//-0.5 sec ассинхронность
	//0,52 просто ассинхронное чтение из кеша
	//2.420000 просто ассинхронное чтение FILE_FLAG_NO_BUFFERING
	//3.082000 просто ассинхронное чтение
	//0,76 выполнение самого кода поиска и работы с копией буфера
	//0,03 блок дозагрузки рабочего буфера межбуферной строкой
	//0,5 find
	//0.52-0,54 весь блок поиска + доп буфер


	std::string sOut;
	std::string sOut2;

	//if (createfile() != 0) { return 1; }; //генератор данных в файле
	clock_t t1;
	clock_t t2;
	//int x;
	//std::string st ("4000000");
	//char str[10];
	char str[] = "4000000";

	//t1 = clock();
	//main0();
	//t2 = clock();
	//printf("main0: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки


	t1 = clock();
	auto tt = SortDeleteDuplicateRowsCSVansi(L"C:\\file.txt", L"C:\\CSV_1_GB2.csv", 0, 0, 0);//"C:\\CSV_1_GB.csv"
	t2 = clock();
	printf("GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//std::cout << tt << std::endl;


	//for (int i = 1; i <= 4000000; i++)
	//{
	//	sOut = FindRowsInCSVansiNew(L"C:\\CSV_1_GB.csv", std::to_string(i).c_str(), 0, 0);
	//	sOut2 = XLAT(std::to_string(i));
	//	if (sOut != sOut2)
	//	{
	//		std::cout <<"find: "<<i<<"\nbedvit\n"<< sOut << std::endl;
	//		std::cout <<"XLAT\n"<<sOut2 << std::endl;
	//	}
	//}

	//std::cout << "\n\FILE_FLAG_NO_BUFFERING\n" << std::endl;
	//for (int i = 1; i <= 5; i++)
	//{
	//	//t1 = clock();
	//	//sOut = FindRowsInCSVansiNew(L"C:\\CSV_1_GB.csv", "4000000", 0, 1);
	//	//t2 = clock();
	//	//printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut2 = XLAT("4000000");
	//	t2 = clock();
	//	printf("XLAT: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut = FindRowsInCSVansi(L"C:\\CSV_1_GB.csv", str, 0, 0);
	//	t2 = clock();
	//	printf("bedvit: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки


	//}

	//кешировано
	//std::cout << "\n\FILE_FLAG_BUFFERING\n" << std::endl;
	//for (int i = 1; i <= 5; i++)
	//{
	//	t1 = clock();
	//	sOut = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv", 0);
	//	t2 = clock();
	//	printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//
	//	t1 = clock();
	//	sOut = GetRowCSVansi(L"C:\\CSV_1_GB.csv", 4000000, 0);
	//	t2 = clock();
	//	printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut = FindRowsInCSVansi(L"C:\\CSV_1_GB.csv", str, 0, 0);
	//	t2 = clock();
	//	printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	//std::cout << sOut << std::endl;
	//	t1 = clock();
	//	sOut2 = XLAT("4000000");
	//	t2 = clock();
	//	printf("XLAT: Time - %f\n\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	//t1 = clock();
	//	//auto O = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv",0);
	//	//t2 = clock();
	//	//printf("GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << O << std::endl;
	//}



	system("pause");
	return 0;
}
